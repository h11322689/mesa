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
      #include "glsl_symbol_table.h"
   #include "glsl_parser_extras.h"
   #include "ir.h"
   #include "program.h"
   #include "util/set.h"
   #include "util/hash_table.h"
   #include "linker.h"
   #include "main/shader_types.h"
      static ir_function_signature *
   find_matching_signature(const char *name, const exec_list *actual_parameters,
            namespace {
      class call_link_visitor : public ir_hierarchical_visitor {
   public:
      call_link_visitor(gl_shader_program *prog, gl_linked_shader *linked,
         {
      this->prog = prog;
   this->shader_list = shader_list;
   this->num_shaders = num_shaders;
   this->success = true;
                        ~call_link_visitor()
   {
                  virtual ir_visitor_status visit(ir_variable *ir)
   {
      _mesa_set_add(locals, ir);
               virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      /* If ir is an ir_call from a function that was imported from another
   * shader callee will point to an ir_function_signature in the original
   * shader.  In this case the function signature MUST NOT BE MODIFIED.
   * Doing so will modify the original shader.  This may prevent that
   * shader from being linkable in other programs.
   */
   const ir_function_signature *const callee = ir->callee;
   assert(callee != NULL);
            /* We don't actually need to find intrinsics; they're not real */
   if (callee->is_intrinsic())
            /* Determine if the requested function signature already exists in the
   * final linked shader.  If it does, use it as the target of the call.
   */
   ir_function_signature *sig =
         ir->callee = sig;
   return visit_continue;
                  /* Try to find the signature in one of the other shaders that is being
   * linked.  If it's not found there, return an error.
   */
   for (unsigned i = 0; i < num_shaders; i++) {
      sig = find_matching_signature(name, &ir->actual_parameters,
         if (sig)
               /* FINISHME: Log the full signature of unresolved function.
         linker_error(this->prog, "unresolved reference to function `%s'\n",
         this->success = false;
   return visit_stop;
                  /* Find the prototype information in the linked shader.  Generate any
   * details that may be missing.
   */
   ir_function *f = linked->symbols->get_function(name);
   f = new(linked) ir_function(name);
      /* Add the new function to the linked IR.  Put it at the end
            * so that it comes after any global variable declarations
      linked->symbols->add_function(f);
   linked->ir->push_tail(f);
                  f->exact_matching_signature(NULL, &callee->parameters);
         linked_sig = new(linked) ir_function_signature(callee->return_type);
   f->add_signature(linked_sig);
                  /* At this point linked_sig and called may be the same.  If ir is an
   * ir_call from linked then linked_sig and callee will be
   * ir_function_signatures that have no definitions (is_defined is false).
   */
   assert(!linked_sig->is_defined);
            /* Create an in-place clone of the function definition.  This multistep
   * process introduces some complexity here, but it has some advantages.
   * The parameter list and the and function body are cloned separately.
   * The clone of the parameter list is used to prime the hashtable used
   * to replace variable references in the cloned body.
   *
   * The big advantage is that the ir_function_signature does not change.
   * This means that we don't have to process the rest of the IR tree to
   * patch ir_call nodes.  In addition, there is no way to remove or
   * replace signature stored in a function.  One could easily be added,
   * but this avoids the need.
   */
            exec_list formal_parameters;
   foreach_in_list(const ir_instruction, original, &sig->parameters) {
               ir_instruction *copy = original->clone(linked, ht);
                                 if (sig->is_defined) {
      foreach_in_list(const ir_instruction, original, &sig->body) {
      ir_instruction *copy = original->clone(linked, ht);
                                    /* Patch references inside the function to things outside the function
   * (i.e., function calls and global variables).
   */
                                 virtual ir_visitor_status visit_leave(ir_call *ir)
   {
      /* Traverse list of function parameters, and for array parameters
   * propagate max_array_access. Otherwise arrays that are only referenced
   * from inside functions via function parameters will be incorrectly
   * optimized. This will lead to incorrect code being generated (or worse).
   * Do it when leaving the node so the children would propagate their
   * array accesses first.
            const exec_node *formal_param_node = ir->callee->parameters.get_head();
   if (formal_param_node) {
      const exec_node *actual_param_node = ir->actual_parameters.get_head();
   while (!actual_param_node->is_tail_sentinel()) {
                                    if (formal_param->type->is_array()) {
      ir_dereference_variable *deref = actual_param->as_dereference_variable();
   if (deref && deref->var && deref->var->type->is_array()) {
      deref->var->data.max_array_access =
      MAX2(formal_param->data.max_array_access,
            }
               virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      /* The non-function variable must be a global, so try to find the
      * variable in the shader's symbol table.  If the variable is not
   * found, then it's a global that *MUST* be defined in the original
   * shader.
      ir_variable *var = linked->symbols->get_variable(ir->var->name);
   if (var == NULL) {
      /* Clone the ir_variable that the dereference already has and add
      * it to the linked shader.
      var = ir->var->clone(linked, NULL);
   linked->symbols->add_variable(var);
      } else {
               if (var->type->is_array()) {
      /* It is possible to have a global array declared in multiple
   * shaders without a size.  The array is implicitly sized by
   * the maximal access to it in *any* shader.  Because of this,
   * we need to track the maximal access to the array as linking
   * pulls more functions in that access the array.
   */
                        if (var->type->length == 0 && ir->var->type->length != 0)
      }
   if (var->is_interface_instance()) {
      /* Similarly, we need implicit sizes of arrays within interface
   * blocks to be sized by the maximal access in *any* shader.
   */
   int *const linked_max_ifc_array_access =
                                       for (unsigned i = 0; i < var->get_interface_type()->length;
      i++) {
   linked_max_ifc_array_access[i] =
         }
      ir->var = var;
                              /** Was function linking successful? */
         private:
      /**
   * Shader program being linked
   *
   * This is only used for logging error messages.
   */
            /** List of shaders available for linking. */
            /** Number of shaders available for linking. */
            /**
   * Final linked shader
   *
   * This is used two ways.  It is used to find global variables in the
   * linked shader that are accessed by the function.  It is also used to add
   * global variables from the shader where the function originated.
   */
            /**
   * Table of variables local to the function.
   */
      };
      } /* anonymous namespace */
      /**
   * Searches a list of shaders for a particular function definition
   */
   ir_function_signature *
   find_matching_signature(const char *name, const exec_list *actual_parameters,
         {
               if (f) {
      ir_function_signature *sig =
            if (sig && (sig->is_defined || sig->is_intrinsic()))
                  }
         bool
   link_function_calls(gl_shader_program *prog, gl_linked_shader *main,
         {
               v.run(main->ir);
      }
