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
   * \file opt_dead_functions.cpp
   *
   * Eliminates unused functions from the linked program.
   */
      #include "ir.h"
   #include "ir_visitor.h"
   #include "ir_expression_flattening.h"
   #include "compiler/glsl_types.h"
      namespace {
      class signature_entry : public exec_node
   {
   public:
      signature_entry(ir_function_signature *sig)
   {
      this->signature = sig;
               ir_function_signature *signature;
      };
      class ir_dead_functions_visitor : public ir_hierarchical_visitor {
   public:
      ir_dead_functions_visitor()
   {
                  ~ir_dead_functions_visitor()
   {
                  virtual ir_visitor_status visit_enter(ir_function_signature *);
                     /* List of signature_entry */
   exec_list signature_list;
      };
      } /* unnamed namespace */
      signature_entry *
   ir_dead_functions_visitor::get_signature_entry(ir_function_signature *sig)
   {
      foreach_in_list(signature_entry, entry, &this->signature_list) {
      return entry;
               signature_entry *entry = new(mem_ctx) signature_entry(sig);
   this->signature_list.push_tail(entry);
      }
         ir_visitor_status
   ir_dead_functions_visitor::visit_enter(ir_function_signature *ir)
   {
               if (strcmp(ir->function_name(), "main") == 0) {
                           }
         ir_visitor_status
   ir_dead_functions_visitor::visit_enter(ir_call *ir)
   {
                           }
      bool
   do_dead_functions(exec_list *instructions)
   {
      ir_dead_functions_visitor v;
                     /* Now that we've figured out which function signatures are used, remove
   * the unused ones, and remove function definitions that have no more
   * signatures.
   */
   foreach_in_list_safe(signature_entry, entry, &v.signature_list) {
      entry->signature->remove();
   delete entry->signature;
   progress = true;
         }
               /* We don't just do this above when we nuked a signature because of
   * const pointers.
   */
   foreach_in_list_safe(ir_instruction, ir, instructions) {
               /* At this point (post-linking), the symbol table is no
      * longer in use, so not removing the function from the
   * symbol table should be OK.
      func->remove();
   delete func;
   progress = true;
                        }
