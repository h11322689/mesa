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
      /**
   * \file opt_tree_grafting.cpp
   *
   * Takes assignments to variables that are dereferenced only once and
   * pastes the RHS expression into where the variable is dereferenced.
   *
   * In the process of various operations like function inlining and
   * tertiary op handling, we'll end up with our expression trees having
   * been chopped up into a series of assignments of short expressions
   * to temps.  Other passes like ir_algebraic.cpp would prefer to see
   * the deepest expression trees they can to try to optimize them.
   *
   * This is a lot like copy propagaton.  In comparison, copy
   * propagation only acts on plain copies, not arbitrary expressions on
   * the RHS.  Generally, we wouldn't want to go pasting some
   * complicated expression everywhere it got used, though, so we don't
   * handle expressions in that pass.
   *
   * The hard part is making sure we don't move an expression across
   * some other assignments that would change the value of the
   * expression.  So we split this into two passes: First, find the
   * variables in our scope which are written to once and read once, and
   * then go through basic blocks seeing if we find an opportunity to
   * move those expressions safely.
   */
      #include "ir.h"
   #include "ir_visitor.h"
   #include "ir_variable_refcount.h"
   #include "ir_basic_block.h"
   #include "ir_optimization.h"
   #include "compiler/glsl_types.h"
      namespace {
      static bool debug = false;
      class ir_tree_grafting_visitor : public ir_hierarchical_visitor {
   public:
      ir_tree_grafting_visitor(ir_assignment *graft_assign,
         {
      this->progress = false;
   this->graft_assign = graft_assign;
               virtual ir_visitor_status visit_leave(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_expression *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_if *);
   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_swizzle *);
                              bool progress;
   ir_variable *graft_var;
      };
      struct find_deref_info {
      ir_variable *var;
      };
      void
   dereferences_variable_callback(ir_instruction *ir, void *data)
   {
      struct find_deref_info *info = (struct find_deref_info *)data;
            if (deref && deref->var == info->var)
      }
      static bool
   dereferences_variable(ir_instruction *ir, ir_variable *var)
   {
               info.var = var;
                        }
      bool
   ir_tree_grafting_visitor::do_graft(ir_rvalue **rvalue)
   {
      if (!*rvalue)
                     if (!deref || deref->var != this->graft_var)
            if (debug) {
      fprintf(stderr, "GRAFTING:\n");
   this->graft_assign->fprint(stderr);
   fprintf(stderr, "\n");
   fprintf(stderr, "TO:\n");
   (*rvalue)->fprint(stderr);
               this->graft_assign->remove();
            this->progress = true;
      }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_loop *ir)
   {
      (void)ir;
   /* Do not traverse into the body of the loop since that is a
   * different basic block.
   */
      }
      /**
   * Check if we can continue grafting after writing to a variable.  If the
   * expression we're trying to graft references the variable, we must stop.
   *
   * \param ir   An instruction that writes to a variable.
   * \param var  The variable being updated.
   */
   ir_visitor_status
   ir_tree_grafting_visitor::check_graft(ir_instruction *ir, ir_variable *var)
   {
      if (dereferences_variable(this->graft_assign->rhs, var)) {
      fprintf(stderr, "graft killed by: ");
   ir->fprint(stderr);
   fprintf(stderr, "\n");
         }
                  }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_leave(ir_assignment *ir)
   {
      if (do_graft(&ir->rhs))
            /* If this assignment updates a variable used in the assignment
   * we're trying to graft, then we're done.
   */
      }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_function *ir)
   {
      (void) ir;
      }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_function_signature *ir)
   {
      (void)ir;
      }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_call *ir)
   {
      foreach_two_lists(formal_node, &ir->callee->parameters,
            ir_variable *sig_param = (ir_variable *) formal_node;
   ir_rvalue *ir = (ir_rvalue *) actual_node;
            if (sig_param->data.mode != ir_var_function_in
   if (check_graft(ir, sig_param) == visit_stop)
         continue;
                  ir->replace_with(new_ir);
   return visit_stop;
                     if (ir->return_deref && check_graft(ir, ir->return_deref->var) == visit_stop)
               }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_expression *ir)
   {
      for (unsigned int i = 0; i < ir->num_operands; i++) {
      return visit_stop;
                  }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_if *ir)
   {
      if (do_graft(&ir->condition))
            /* Do not traverse into the body of the if-statement since that is a
   * different basic block.
   */
      }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_swizzle *ir)
   {
      if (do_graft(&ir->val))
               }
      ir_visitor_status
   ir_tree_grafting_visitor::visit_enter(ir_texture *ir)
   {
      if (do_graft(&ir->coordinate) ||
      do_graft(&ir->projector) ||
   do_graft(&ir->offset) ||
   do_graft(&ir->shadow_comparator) ||
   return visit_stop;
         switch (ir->op) {
   case ir_tex:
   case ir_lod:
   case ir_query_levels:
   case ir_texture_samples:
   case ir_samples_identical:
         case ir_txb:
      return visit_stop;
            case ir_txf:
   case ir_txl:
   case ir_txs:
      return visit_stop;
            case ir_txf_ms:
      if (do_graft(&ir->lod_info.sample_index))
            case ir_txd:
            return visit_stop;
            case ir_tg4:
      if (do_graft(&ir->lod_info.component))
                        }
      struct tree_grafting_info {
      ir_variable_refcount_visitor *refs;
      };
      static bool
   try_tree_grafting(ir_assignment *start,
      ir_variable *lhs_var,
      {
               if (debug) {
      fprintf(stderr, "trying to graft: ");
   lhs_var->fprint(stderr);
                  ir != bb_last->next;
   ir = (ir_instruction *)ir->next) {
            fprintf(stderr, "- ");
   ir->fprint(stderr);
   fprintf(stderr, "\n");
                  ir_visitor_status s = ir->accept(&v);
   return v.progress;
                  }
      static void
   tree_grafting_basic_block(ir_instruction *bb_first,
      ir_instruction *bb_last,
      {
      struct tree_grafting_info *info = (struct tree_grafting_info *)data;
               ir != bb_last->next;
   ir = next, next = (ir_instruction *)ir->next) {
                  continue;
            ir_variable *lhs_var = assign->whole_variable_written();
   continue;
            if (lhs_var->data.mode == ir_var_function_out ||
      lhs_var->data.mode == ir_var_function_inout ||
   lhs_var->data.mode == ir_var_shader_out ||
   lhs_var->data.mode == ir_var_shader_storage ||
               if (lhs_var->data.precise)
            /* Do not graft sampler and image variables. This is a workaround to
   * st/glsl_to_tgsi being unable to handle expression parameters to image
   * intrinsics.
   *
   * Note that if this is ever fixed, we still need to skip grafting when
   * any image layout qualifiers (including the image format) are set,
   * since we must not lose those.
   */
   if (lhs_var->type->is_sampler() || lhs_var->type->is_image())
                        entry->assigned_count != 1 ||
            continue;
            /* Found a possibly graftable assignment.  Now, walk through the
   * rest of the BB seeing if the deref is here, and if nothing interfered with
   * pasting its expression's values in between.
   */
         }
      } /* unnamed namespace */
      /**
   * Does a copy propagation pass on the code present in the instruction stream.
   */
   bool
   do_tree_grafting(exec_list *instructions)
   {
      ir_variable_refcount_visitor refs;
            info.progress = false;
                                 }
