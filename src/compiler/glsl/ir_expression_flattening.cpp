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
   * \file ir_expression_flattening.cpp
   *
   * Takes the leaves of expression trees and makes them dereferences of
   * assignments of the leaves to temporaries, according to a predicate.
   *
   * This is used for breaking down matrix operations, where it's easier to
   * create a temporary and work on each of its vector components individually.
   */
      #include "ir.h"
   #include "ir_rvalue_visitor.h"
   #include "ir_expression_flattening.h"
      class ir_expression_flattening_visitor : public ir_rvalue_visitor {
   public:
      ir_expression_flattening_visitor(bool (*predicate)(ir_instruction *ir))
   {
                  virtual ~ir_expression_flattening_visitor()
   {
                  void handle_rvalue(ir_rvalue **rvalue);
      };
      void
   do_expression_flattening(exec_list *instructions,
         {
               foreach_in_list(ir_instruction, ir, instructions) {
            }
      void
   ir_expression_flattening_visitor::handle_rvalue(ir_rvalue **rvalue)
   {
      ir_variable *var;
   ir_assignment *assign;
            if (!ir || !this->predicate(ir))
                     var = new(ctx) ir_variable(ir->type, "flattening_tmp", ir_var_temporary);
            assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(var), ir);
               }
