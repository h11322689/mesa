   /*
   * Copyright © 2016 Intel Corporation
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
      #include <inttypes.h> /* for PRIx64 macro */
   #include "ir.h"
   #include "ir_hierarchical_visitor.h"
   #include "ir_builder_print_visitor.h"
   #include "compiler/glsl_types.h"
   #include "glsl_parser_extras.h"
   #include "main/macros.h"
   #include "util/hash_table.h"
   #include "util/u_string.h"
      class ir_builder_print_visitor : public ir_hierarchical_visitor {
   public:
      ir_builder_print_visitor(FILE *f);
                     virtual ir_visitor_status visit(class ir_variable *);
   virtual ir_visitor_status visit(class ir_dereference_variable *);
   virtual ir_visitor_status visit(class ir_constant *);
                     virtual ir_visitor_status visit_enter(class ir_loop *);
            virtual ir_visitor_status visit_enter(class ir_function_signature *);
                     virtual ir_visitor_status visit_enter(class ir_assignment *);
            virtual ir_visitor_status visit_leave(class ir_call *);
   virtual ir_visitor_status visit_leave(class ir_swizzle *);
                  private:
      void print_with_indent(const char *fmt, ...);
            void print_without_declaration(const ir_rvalue *ir);
   void print_without_declaration(const ir_constant *ir);
   void print_without_declaration(const ir_dereference_variable *ir);
   void print_without_declaration(const ir_swizzle *ir);
                     /**
   * Mapping from ir_instruction * -> index used in the generated C code
   * variable name.
   */
                        };
      /* An operand is "simple" if it can be compactly printed on one line.
   */
   static bool
   is_simple_operand(const ir_rvalue *ir, unsigned depth = 1)
   {
      if (depth == 0)
            switch (ir->ir_type) {
   case ir_type_dereference_variable:
            case ir_type_constant: {
      if (ir->type == glsl_type::uint_type ||
      ir->type == glsl_type::int_type ||
   ir->type == glsl_type::float_type ||
               const ir_constant *const c = (ir_constant *) ir;
   ir_constant_data all_zero;
                        case ir_type_swizzle: {
      const ir_swizzle *swiz = (ir_swizzle *) ir;
   return swiz->mask.num_components == 1 &&
               case ir_type_expression: {
               for (unsigned i = 0; i < expr->num_operands; i++) {
      if (!is_simple_operand(expr->operands[i], depth - 1))
                           default:
            }
      void
   _mesa_print_builder_for_ir(FILE *f, exec_list *instructions)
   {
      ir_builder_print_visitor v(f);
      }
      ir_builder_print_visitor::ir_builder_print_visitor(FILE *f)
         {
         }
      ir_builder_print_visitor::~ir_builder_print_visitor()
   {
         }
      void ir_builder_print_visitor::indent(void)
   {
      for (int i = 0; i < indentation; i++)
      }
      void
   ir_builder_print_visitor::print_with_indent(const char *fmt, ...)
   {
                        va_start(ap, fmt);
   vfprintf(f, fmt, ap);
      }
      void
   ir_builder_print_visitor::print_without_indent(const char *fmt, ...)
   {
               va_start(ap, fmt);
   vfprintf(f, fmt, ap);
      }
      void
   ir_builder_print_visitor::print_without_declaration(const ir_rvalue *ir)
   {
      switch (ir->ir_type) {
   case ir_type_dereference_variable:
      print_without_declaration((ir_dereference_variable *) ir);
      case ir_type_constant:
      print_without_declaration((ir_constant *) ir);
      case ir_type_swizzle:
      print_without_declaration((ir_swizzle *) ir);
      case ir_type_expression:
      print_without_declaration((ir_expression *) ir);
      default:
            }
      ir_visitor_status
   ir_builder_print_visitor::visit(ir_variable *ir)
   {
                        const char *mode_str;
   switch (ir->data.mode) {
   case ir_var_auto: mode_str = "ir_var_auto"; break;
   case ir_var_uniform: mode_str = "ir_var_uniform"; break;
   case ir_var_shader_storage: mode_str = "ir_var_shader_storage"; break;
   case ir_var_shader_shared: mode_str = "ir_var_shader_shared"; break;
   case ir_var_shader_in: mode_str = "ir_var_shader_in"; break;
   case ir_var_shader_out: mode_str = "ir_var_shader_out"; break;
   case ir_var_function_in: mode_str = "ir_var_function_in"; break;
   case ir_var_function_out: mode_str = "ir_var_function_out"; break;
   case ir_var_function_inout: mode_str = "ir_var_function_inout"; break;
   case ir_var_const_in: mode_str = "ir_var_const_in"; break;
   case ir_var_system_value: mode_str = "ir_var_system_value"; break;
   case ir_var_temporary: mode_str = "ir_var_temporary"; break;
   default:
                  if (ir->data.mode == ir_var_temporary) {
      print_with_indent("ir_variable *const r%04X = body.make_temp(glsl_type::%s_type, \"%s\");\n",
                  } else {
      print_with_indent("ir_variable *const r%04X = new(mem_ctx) ir_variable(glsl_type::%s_type, \"%s\", %s);\n",
                              switch (ir->data.mode) {
   case ir_var_function_in:
   case ir_var_function_out:
   case ir_var_function_inout:
   case ir_var_const_in:
      print_with_indent("sig_parameters.push_tail(r%04X);\n", my_index);
      default:
      print_with_indent("body.emit(r%04X);\n", my_index);
                     }
      void
   ir_builder_print_visitor::print_without_declaration(const ir_dereference_variable *ir)
   {
      const struct hash_entry *const he =
               }
      ir_visitor_status
   ir_builder_print_visitor::visit(ir_dereference_variable *ir)
   {
      const struct hash_entry *const he =
            if (he != NULL)
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_enter(ir_function_signature *ir)
   {
      if (!ir->is_defined)
            print_with_indent("ir_function_signature *\n"
                     indentation++;
   print_with_indent("ir_function_signature *const sig =\n");
   print_with_indent("   new(mem_ctx) ir_function_signature(glsl_type::%s_type, avail);\n",
            print_with_indent("ir_factory body(&sig->body, mem_ctx);\n");
            if (!ir->parameters.is_empty())
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_leave(ir_function_signature *ir)
   {
      if (!ir->parameters.is_empty())
            print_with_indent("return sig;\n");
   indentation--;
   print_with_indent("}\n");
      }
      void
   ir_builder_print_visitor::print_without_declaration(const ir_constant *ir)
   {
   if (ir->type->is_scalar()) {
         switch (ir->type->base_type) {
   case GLSL_TYPE_UINT:
      print_without_indent("body.constant(%uu)", ir->value.u[0]);
      case GLSL_TYPE_INT:
      print_without_indent("body.constant(int(%d))", ir->value.i[0]);
      case GLSL_TYPE_FLOAT:
      print_without_indent("body.constant(%ff)", ir->value.f[0]);
      case GLSL_TYPE_BOOL:
      print_without_indent("body.constant(%s)",
            default:
                     ir_constant_data all_zero;
            if (memcmp(&ir->value, &all_zero, sizeof(all_zero)) == 0) {
      print_without_indent("ir_constant::zero(mem_ctx, glsl_type::%s_type)",
         }
      ir_visitor_status
   ir_builder_print_visitor::visit(ir_constant *ir)
   {
                        if (ir->type == glsl_type::uint_type ||
      ir->type == glsl_type::int_type ||
   ir->type == glsl_type::float_type ||
   ir->type == glsl_type::bool_type) {
   print_with_indent("ir_constant *const r%04X = ", my_index);
   print_without_declaration(ir);
   print_without_indent(";\n");
               ir_constant_data all_zero;
            if (memcmp(&ir->value, &all_zero, sizeof(all_zero)) == 0) {
      print_with_indent("ir_constant *const r%04X = ", my_index);
   print_without_declaration(ir);
      } else {
      print_with_indent("ir_constant_data r%04X_data;\n", my_index);
   print_with_indent("memset(&r%04X_data, 0, sizeof(ir_constant_data));\n",
         for (unsigned i = 0; i < 16; i++) {
      switch (ir->type->base_type) {
   case GLSL_TYPE_UINT:
      if (ir->value.u[i] != 0)
      print_with_indent("r%04X_data.u[%u] = %u;\n",
         case GLSL_TYPE_INT:
      if (ir->value.i[i] != 0)
      print_with_indent("r%04X_data.i[%u] = %i;\n",
         case GLSL_TYPE_FLOAT:
      if (ir->value.u[i] != 0)
      print_with_indent("r%04X_data.u[%u] = 0x%08x; /* %f */\n",
                                                      memcpy(&v, &ir->value.d[i], sizeof(v));
   if (v != 0)
      print_with_indent("r%04X_data.u64[%u] = 0x%016" PRIx64 "; /* %g */\n",
         }
   case GLSL_TYPE_UINT64:
      if (ir->value.u64[i] != 0)
      print_with_indent("r%04X_data.u64[%u] = %" PRIu64 ";\n",
                     case GLSL_TYPE_INT64:
      if (ir->value.i64[i] != 0)
      print_with_indent("r%04X_data.i64[%u] = %" PRId64 ";\n",
                     case GLSL_TYPE_BOOL:
      if (ir->value.u[i] != 0)
            default:
                     print_with_indent("ir_constant *const r%04X = new(mem_ctx) ir_constant(glsl_type::%s_type, &r%04X_data);\n",
                              }
      void
   ir_builder_print_visitor::print_without_declaration(const ir_swizzle *ir)
   {
      const struct hash_entry *const he =
            if (ir->mask.num_components == 1) {
               if (is_simple_operand(ir->val)) {
      print_without_indent("swizzle_%c(", swiz[ir->mask.x]);
   print_without_declaration(ir->val);
      } else {
      assert(he);
   print_without_indent("swizzle_%c(r%04X)",
               } else {
               assert(he);
   print_without_indent("swizzle(r%04X, MAKE_SWIZZLE4(SWIZZLE_%c, SWIZZLE_%c, SWIZZLE_%c, SWIZZLE_%c), %u)",
                        (unsigned)(uintptr_t) he->data,
   swiz[ir->mask.x],
      }
      ir_visitor_status
   ir_builder_print_visitor::visit_leave(ir_swizzle *ir)
   {
                        print_with_indent("ir_swizzle *const r%04X = ", my_index);
   print_without_declaration(ir);
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_enter(ir_assignment *ir)
   {
               if (!is_simple_operand(ir->rhs) && rhs_expr == NULL)
            if (rhs_expr != NULL) {
               for (unsigned i = 0; i < num_op; i++) {
                                             this->in_assignee = true;
   s = ir->lhs->accept(this);
   this->in_assignee = false;
   if (s != visit_continue)
            const struct hash_entry *const he_lhs =
            print_with_indent("body.emit(assign(r%04X, ",
         print_without_declaration(ir->rhs);
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_leave(ir_assignment *ir)
   {
      const struct hash_entry *const he_lhs =
            const struct hash_entry *const he_rhs =
                     print_with_indent("body.emit(assign(r%04X, r%04X, 0x%02x));\n\n",
                           }
      void
   ir_builder_print_visitor::print_without_declaration(const ir_expression *ir)
   {
               static const char *const arity[] = {
                  switch (ir->operation) {
   case ir_unop_neg:
   case ir_binop_add:
   case ir_binop_sub:
   case ir_binop_mul:
   case ir_binop_imul_high:
   case ir_binop_less:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_nequal:
   case ir_binop_lshift:
   case ir_binop_rshift:
   case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
   case ir_binop_logic_and:
   case ir_binop_logic_xor:
   case ir_binop_logic_or:
      print_without_indent("%s(",
            default:
      print_without_indent("expr(ir_%s_%s, ",
                           for (unsigned i = 0; i < num_op; i++) {
      if (is_simple_operand(ir->operands[i]))
         else {
                                 if (i < num_op - 1)
                  }
      ir_visitor_status
   ir_builder_print_visitor::visit_enter(ir_expression *ir)
   {
               for (unsigned i = 0; i < num_op; i++) {
      if (is_simple_operand(ir->operands[i]))
                                          print_with_indent("ir_expression *const r%04X = ", my_index);
   print_without_declaration(ir);
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_enter(ir_if *ir)
   {
                        ir_visitor_status s = ir->condition->accept(this);
   if (s != visit_continue)
            const struct hash_entry *const he =
            print_with_indent("ir_if *f%04X = new(mem_ctx) ir_if(operand(r%04X).val);\n",
               print_with_indent("exec_list *const f%04X_parent_instructions = body.instructions;\n\n",
            indentation++;
   print_with_indent("/* THEN INSTRUCTIONS */\n");
   print_with_indent("body.instructions = &f%04X->then_instructions;\n\n",
            if (s != visit_continue_with_parent) {
      s = visit_list_elements(this, &ir->then_instructions);
   if (s == visit_stop)
                        if (!ir->else_instructions.is_empty()) {
      print_with_indent("/* ELSE INSTRUCTIONS */\n");
   print_with_indent("body.instructions = &f%04X->else_instructions;\n\n",
            if (s != visit_continue_with_parent) {
      s = visit_list_elements(this, &ir->else_instructions);
   if (s == visit_stop)
                                    print_with_indent("body.instructions = f%04X_parent_instructions;\n",
         print_with_indent("body.emit(f%04X);\n\n",
                     }
      ir_visitor_status
   ir_builder_print_visitor::visit_leave(ir_return *ir)
   {
      const struct hash_entry *const he =
            print_with_indent("body.emit(ret(r%04X));\n\n",
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_enter(ir_texture *ir)
   {
                  }
      ir_visitor_status
   ir_builder_print_visitor::visit_leave(ir_call *ir)
   {
               print_without_indent("\n");
   print_with_indent("/* CALL %s */\n", ir->callee_name());
            foreach_in_list(ir_dereference_variable, param, &ir->actual_parameters) {
      const struct hash_entry *const he =
            print_with_indent("r%04X_parameters.push_tail(operand(r%04X).val);\n",
                     char return_deref_string[32];
   if (ir->return_deref) {
      const struct hash_entry *const he =
            snprintf(return_deref_string, sizeof(return_deref_string),
      } else {
                  print_with_indent("body.emit(new(mem_ctx) ir_call(shader->symbols->get_function(\"%s\"),\n",
         print_with_indent("                               %s, &r%04X_parameters);\n\n",
                  }
      ir_visitor_status
   ir_builder_print_visitor::visit_enter(ir_loop *ir)
   {
                        print_with_indent("/* LOOP BEGIN */\n");
   print_with_indent("ir_loop *f%04X = new(mem_ctx) ir_loop();\n", my_index);
   print_with_indent("exec_list *const f%04X_parent_instructions = body.instructions;\n\n",
                     print_with_indent("body.instructions = &f%04X->body_instructions;\n\n",
               }
      ir_visitor_status
   ir_builder_print_visitor::visit_leave(ir_loop *ir)
   {
      const struct hash_entry *const he =
                     print_with_indent("/* LOOP END */\n\n");
   print_with_indent("body.instructions = f%04X_parent_instructions;\n",
         print_with_indent("body.emit(f%04X);\n\n",
               }
      ir_visitor_status
   ir_builder_print_visitor::visit(ir_loop_jump *ir)
   {
      print_with_indent("body.emit(new(mem_ctx) ir_loop_jump(ir_loop_jump::jump_%s));\n\n",
            }
