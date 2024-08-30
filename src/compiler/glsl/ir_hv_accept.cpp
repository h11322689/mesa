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
      #include "ir.h"
      /**
   * \file ir_hv_accept.cpp
   * Implementations of all hierarchical visitor accept methods for IR
   * instructions.
   */
      /**
   * Process a list of nodes using a hierarchical vistor.
   *
   * If statement_list is true (the default), this is a list of statements, so
   * v->base_ir will be set to point to each statement just before iterating
   * over it, and restored after iteration is complete.  If statement_list is
   * false, this is a list that appears inside a statement (e.g. a parameter
   * list), so v->base_ir will be left alone.
   *
   * \warning
   * This function will operate correctly if a node being processed is removed
   * from the list.  However, if nodes are added to the list after the node being
   * processed, some of the added nodes may not be processed.
   */
   ir_visitor_status
   visit_list_elements(ir_hierarchical_visitor *v, exec_list *l,
         {
               foreach_in_list_safe(ir_instruction, ir, l) {
      if (statement_list)
                  return s;
      }
   if (statement_list)
               }
         ir_visitor_status
   ir_rvalue::accept(ir_hierarchical_visitor *v)
   {
         }
         ir_visitor_status
   ir_variable::accept(ir_hierarchical_visitor *v)
   {
         }
         ir_visitor_status
   ir_loop::accept(ir_hierarchical_visitor *v)
   {
               if (s != visit_continue)
            s = visit_list_elements(v, &this->body_instructions);
   if (s == visit_stop)
               }
         ir_visitor_status
   ir_loop_jump::accept(ir_hierarchical_visitor *v)
   {
         }
         ir_visitor_status
   ir_function_signature::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = visit_list_elements(v, &this->parameters);
   if (s == visit_stop)
            s = visit_list_elements(v, &this->body);
      }
         ir_visitor_status
   ir_function::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = visit_list_elements(v, &this->signatures, false);
      }
         ir_visitor_status
   ir_expression::accept(ir_hierarchical_visitor *v)
   {
               if (s != visit_continue)
            for (unsigned i = 0; i < this->num_operands; i++) {
      switch (this->operands[i]->accept(v)) {
   break;
            // I wish for Java's labeled break-statement here.
   goto done;
            return visit_stop;
                  done:
         }
      ir_visitor_status
   ir_texture::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = this->sampler->accept(v);
   if (s != visit_continue)
            if (this->coordinate) {
      s = this->coordinate->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
               if (this->projector) {
      s = this->projector->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
               if (this->shadow_comparator) {
      s = this->shadow_comparator->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
               if (this->offset) {
      s = this->offset->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
               if (this->clamp) {
      s = this->clamp->accept(v);
   if (s != visit_continue)
               switch (this->op) {
   case ir_tex:
   case ir_lod:
   case ir_query_levels:
   case ir_texture_samples:
   case ir_samples_identical:
         case ir_txb:
      s = this->lod_info.bias->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
            case ir_txl:
   case ir_txf:
   case ir_txs:
      s = this->lod_info.lod->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
            case ir_txf_ms:
      s = this->lod_info.sample_index->accept(v);
   if (s != visit_continue)
            case ir_txd:
      s = this->lod_info.grad.dPdx->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
            s = this->lod_info.grad.dPdy->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
            case ir_tg4:
      s = this->lod_info.component->accept(v);
   if (s != visit_continue)
                     assert(s == visit_continue);
      }
         ir_visitor_status
   ir_swizzle::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = this->val->accept(v);
      }
         ir_visitor_status
   ir_dereference_variable::accept(ir_hierarchical_visitor *v)
   {
         }
         ir_visitor_status
   ir_dereference_array::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            /* The array index is not the target of the assignment, so clear the
   * 'in_assignee' flag.  Restore it after returning from the array index.
   */
   const bool was_in_assignee = v->in_assignee;
   v->in_assignee = false;
   s = this->array_index->accept(v);
            if (s != visit_continue)
            s = this->array->accept(v);
      }
         ir_visitor_status
   ir_dereference_record::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = this->record->accept(v);
      }
         ir_visitor_status
   ir_assignment::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            v->in_assignee = true;
   s = this->lhs->accept(v);
   v->in_assignee = false;
   if (s != visit_continue)
            s = this->rhs->accept(v);
   if (s != visit_continue)
               }
         ir_visitor_status
   ir_constant::accept(ir_hierarchical_visitor *v)
   {
         }
         ir_visitor_status
   ir_call::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            if (this->return_deref != NULL) {
      v->in_assignee = true;
   s = this->return_deref->accept(v);
   v->in_assignee = false;
   return (s == visit_continue_with_parent) ? visit_continue : s;
               s = visit_list_elements(v, &this->actual_parameters, false);
   if (s == visit_stop)
               }
         ir_visitor_status
   ir_return::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            ir_rvalue *val = this->get_value();
   if (val) {
      s = val->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
                  }
         ir_visitor_status
   ir_discard::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            if (this->condition != NULL) {
      s = this->condition->accept(v);
   return (s == visit_continue_with_parent) ? visit_continue : s;
                  }
         ir_visitor_status
   ir_demote::accept(ir_hierarchical_visitor *v)
   {
               if (s != visit_continue)
               }
         ir_visitor_status
   ir_if::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = this->condition->accept(v);
   if (s != visit_continue)
            if (s != visit_continue_with_parent) {
      s = visit_list_elements(v, &this->then_instructions);
   return s;
               if (s != visit_continue_with_parent) {
      s = visit_list_elements(v, &this->else_instructions);
   return s;
                  }
      ir_visitor_status
   ir_emit_vertex::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = this->stream->accept(v);
   if (s != visit_continue)
            assert(s == visit_continue);
      }
         ir_visitor_status
   ir_end_primitive::accept(ir_hierarchical_visitor *v)
   {
      ir_visitor_status s = v->visit_enter(this);
   if (s != visit_continue)
            s = this->stream->accept(v);
   if (s != visit_continue)
            assert(s == visit_continue);
      }
      ir_visitor_status
   ir_barrier::accept(ir_hierarchical_visitor *v)
   {
         }
