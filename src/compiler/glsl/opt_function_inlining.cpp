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
   * \file opt_function_inlining.cpp
   *
   * Replaces calls to functions with the body of the function.
   */
      #include "ir.h"
   #include "ir_visitor.h"
   #include "ir_rvalue_visitor.h"
   #include "ir_function_inlining.h"
   #include "ir_expression_flattening.h"
   #include "compiler/glsl_types.h"
   #include "util/hash_table.h"
      static void
   do_variable_replacement(exec_list *instructions,
                  namespace {
      class ir_function_inlining_visitor : public ir_hierarchical_visitor {
   public:
      ir_function_inlining_visitor()
   {
                  virtual ~ir_function_inlining_visitor()
   {
                  virtual ir_visitor_status visit_enter(ir_expression *);
   virtual ir_visitor_status visit_enter(ir_call *);
   virtual ir_visitor_status visit_enter(ir_return *);
   virtual ir_visitor_status visit_enter(ir_texture *);
               };
      class ir_save_lvalue_visitor : public ir_hierarchical_visitor {
   public:
         };
      } /* unnamed namespace */
      bool
   do_function_inlining(exec_list *instructions)
   {
                           }
      static void
   replace_return_with_assignment(ir_instruction *ir, void *data)
   {
      void *ctx = ralloc_parent(ir);
   ir_dereference *orig_deref = (ir_dereference *) data;
            if (ret) {
      ir_rvalue *lhs = orig_deref->clone(ctx, NULL);
               /* un-valued return has to be the last return, or we shouldn't
      * have reached here. (see can_inline()).
      assert(ret->next->is_tail_sentinel());
   ret->remove();
               }
      /* Save the given lvalue before the given instruction.
   *
   * This is done by adding temporary variables into which the current value
   * of any array indices are saved, and then modifying the dereference chain
   * in-place to point to those temporary variables.
   *
   * The hierarchical visitor is only used to traverse the left-hand-side chain
   * of derefs.
   */
   ir_visitor_status
   ir_save_lvalue_visitor::visit_enter(ir_dereference_array *deref)
   {
      if (deref->array_index->ir_type != ir_type_constant) {
      void *ctx = ralloc_parent(deref);
   ir_variable *index;
            index = new(ctx) ir_variable(deref->array_index->type, "saved_idx", ir_var_temporary);
            assignment = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(index),
                              deref->array->accept(this);
      }
      static bool
   should_replace_variable(ir_variable *sig_param, ir_rvalue *param,
               if (sig_param->data.mode != ir_var_function_in &&
      sig_param->data.mode != ir_var_const_in)
         /* Some places in glsl_to_nir() expect images to always be copied to a temp
   * first.
   */
   if (sig_param->type->without_array()->is_image() && !param->is_dereference())
            /* SSBO and shared vars might be passed to a built-in such as an atomic
   * memory function, where copying these to a temp before passing to the
   * atomic function is not valid so we must replace these instead. Also,
   * shader inputs for interpolateAt funtions also need to be replaced.
   *
   * Our builtins should always use temps and not the inputs themselves to
   * store temporay values so just checking is_builtin rather than string
   * comparing the function name for e.g atomic* should always be safe.
   */
   if (is_builtin)
            /* For opaque types, we want the inlined variable references
   * referencing the passed in variable, since that will have
   * the location information, which an assignment of an opaque
   * variable wouldn't.
   */
      }
      void
   ir_call::generate_inline(ir_instruction *next_ir)
   {
      void *ctx = ralloc_parent(this);
   ir_variable **parameters;
   unsigned num_parameters;
   int i;
                     num_parameters = this->callee->parameters.length();
            /* Generate the declarations for the parameters to our inlined code,
   * and set up the mapping of real function body variables to ours.
   */
   i = 0;
   foreach_two_lists(formal_node, &this->callee->parameters,
            ir_variable *sig_param = (ir_variable *) formal_node;
            /* Generate a new variable for the parameter. */
   if (should_replace_variable(sig_param, param,
         parameters[i] = NULL;
         parameters[i] = sig_param->clone(ctx, ht);
   parameters[i]->data.mode = ir_var_temporary;
      /* Remove the read-only decoration because we're going to write
      * directly to this variable.  If the cloned variable is left
   * read-only and the inlined function is inside a loop, the loop
   * analysis code will get confused.
      parameters[i]->data.read_only = false;
   next_ir->insert_before(parameters[i]);
                  /* Section 6.1.1 (Function Calling Conventions) of the OpenGL Shading
   * Language 4.5 spec says:
   *
   *    "All arguments are evaluated at call time, exactly once, in order,
   *     from left to right. [...] Evaluation of an out parameter results
   *     in an l-value that is used to copy out a value when the function
   *     returns."
   *
   * I.e., we have to take temporary copies of any relevant array indices
   * before the function body is executed.
   *
   * This ensures that
   * (a) if an array index expressions refers to a variable that is
   *     modified by the execution of the function body, we use the
   *     original value as intended, and
   * (b) if an array index expression has side effects, those side effects
   *     are only executed once and at the right time.
   */
   if (parameters[i]) {
      if (sig_param->data.mode == ir_var_function_in ||
                     assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(parameters[i]),
            } else {
      assert(sig_param->data.mode == ir_var_function_out ||
                                                            assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(parameters[i]),
                                                /* Generate the inlined body of the function to a new list */
   foreach_in_list(ir_instruction, ir, &callee->body) {
               new_instructions.push_tail(new_ir);
               /* If any opaque types were passed in, replace any deref of the
   * opaque variable with a deref of the argument.
   */
   foreach_two_lists(formal_node, &this->callee->parameters,
            ir_rvalue *const param = (ir_rvalue *) actual_node;
            if (should_replace_variable(sig_param, param,
                           /* Now push those new instructions in. */
            /* Copy back the value of any 'out' parameters from the function body
   * variables to our own.
   */
   i = 0;
   foreach_two_lists(formal_node, &this->callee->parameters,
            ir_rvalue *const param = (ir_rvalue *) actual_node;
            /* Move our param variable into the actual param if it's an 'out' type. */
   if (parameters[i] && (sig_param->data.mode == ir_var_function_out ||
   ir_assignment *assign;
                  next_ir->insert_before(assign);
                                          }
         ir_visitor_status
   ir_function_inlining_visitor::visit_enter(ir_expression *ir)
   {
      (void) ir;
      }
         ir_visitor_status
   ir_function_inlining_visitor::visit_enter(ir_return *ir)
   {
      (void) ir;
      }
         ir_visitor_status
   ir_function_inlining_visitor::visit_enter(ir_texture *ir)
   {
      (void) ir;
      }
         ir_visitor_status
   ir_function_inlining_visitor::visit_enter(ir_swizzle *ir)
   {
      (void) ir;
      }
         ir_visitor_status
   ir_function_inlining_visitor::visit_enter(ir_call *ir)
   {
      if (can_inline(ir)) {
      ir->generate_inline(ir);
   ir->remove();
                  }
         /**
   * Replaces references to the "orig" variable with a clone of "repl."
   *
   * From the spec, opaque types can appear in the tree as function
   * (non-out) parameters and as the result of array indexing and
   * structure field selection.  In our builtin implementation, they
   * also appear in the sampler field of an ir_tex instruction.
   */
      class ir_variable_replacement_visitor : public ir_rvalue_visitor {
   public:
      ir_variable_replacement_visitor(ir_variable *orig, ir_rvalue *repl)
   {
      this->orig = orig;
               virtual ~ir_variable_replacement_visitor()
   {
            virtual ir_visitor_status visit_leave(ir_call *);
   virtual ir_visitor_status visit_leave(ir_texture *);
            void handle_rvalue(ir_rvalue **rvalue);
   void replace_deref(ir_dereference **deref);
            ir_variable *orig;
      };
      void
   ir_variable_replacement_visitor::replace_deref(ir_dereference **deref)
   {
      ir_dereference_variable *deref_var = (*deref)->as_dereference_variable();
   if (deref_var && deref_var->var == this->orig)
      }
      void
   ir_variable_replacement_visitor::handle_rvalue(ir_rvalue **rvalue)
   {
         }
      void
   ir_variable_replacement_visitor::replace_rvalue(ir_rvalue **rvalue)
   {
      if (!*rvalue)
                     if (!deref)
            ir_dereference_variable *deref_var = (deref)->as_dereference_variable();
   if (deref_var && deref_var->var == this->orig)
      }
      ir_visitor_status
   ir_variable_replacement_visitor::visit_leave(ir_texture *ir)
   {
                  }
      ir_visitor_status
   ir_variable_replacement_visitor::visit_leave(ir_assignment *ir)
   {
      replace_deref(&ir->lhs);
               }
      ir_visitor_status
   ir_variable_replacement_visitor::visit_leave(ir_call *ir)
   {
      foreach_in_list_safe(ir_rvalue, param, &ir->actual_parameters) {
      ir_rvalue *new_param = param;
            if (new_param != param) {
            }
      }
      static void
   do_variable_replacement(exec_list *instructions,
               {
                  }
