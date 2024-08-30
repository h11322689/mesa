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
   * \file ir_validate.cpp
   *
   * Attempts to verify that various invariants of the IR tree are true.
   *
   * In particular, at the moment it makes sure that no single
   * ir_instruction node except for ir_variable appears multiple times
   * in the ir tree.  ir_variable does appear multiple times: Once as a
   * declaration in an exec_list, and multiple times as the endpoint of
   * a dereference chain.
   */
      #include "ir.h"
   #include "ir_hierarchical_visitor.h"
   #include "linker_util.h"
   #include "util/u_debug.h"
   #include "util/hash_table.h"
   #include "util/macros.h"
   #include "util/set.h"
   #include "compiler/glsl_types.h"
      namespace {
      class ir_validate : public ir_hierarchical_visitor {
   public:
      ir_validate()
   {
                        this->callback_enter = ir_validate::validate_ir;
               ~ir_validate()
   {
                  virtual ir_visitor_status visit(ir_variable *v);
            virtual ir_visitor_status visit_enter(ir_discard *ir);
            virtual ir_visitor_status visit_enter(ir_function *ir);
   virtual ir_visitor_status visit_leave(ir_function *ir);
   virtual ir_visitor_status visit_enter(ir_function_signature *ir);
            virtual ir_visitor_status visit_leave(ir_expression *ir);
            virtual ir_visitor_status visit_enter(class ir_dereference_array *);
            virtual ir_visitor_status visit_enter(ir_assignment *ir);
                                 };
      } /* anonymous namespace */
      ir_visitor_status
   ir_validate::visit(ir_dereference_variable *ir)
   {
      if ((ir->var == NULL) || (ir->var->as_variable() == NULL)) {
      printf("ir_dereference_variable @ %p does not specify a variable %p\n",
   (void *) ir, (void *) ir->var);
               /* Compare types without arrays, because one side can be sized and
   * the other unsized.
   */
   if (ir->var->type->without_array() != ir->type->without_array()) {
      printf("ir_dereference_variable type is not equal to variable type: ");
   ir->print();
   printf("\n");
               if (_mesa_set_search(ir_set, ir->var) == NULL) {
      printf("ir_dereference_variable @ %p specifies undeclared variable "
   "`%s' @ %p\n",
   (void *) ir, ir->var->name, (void *) ir->var);
                           }
      ir_visitor_status
   ir_validate::visit_enter(class ir_dereference_array *ir)
   {
      if (!ir->array->type->is_array() && !ir->array->type->is_matrix() &&
      !ir->array->type->is_vector()) {
   printf("ir_dereference_array @ %p does not specify an array, a vector "
         "or a matrix\n",
   ir->print();
   printf("\n");
               if (ir->array->type->is_array()) {
      if (ir->array->type->fields.array != ir->type) {
      printf("ir_dereference_array type is not equal to the array "
         ir->print();
   printf("\n");
         } else if (ir->array->type->base_type != ir->type->base_type) {
      printf("ir_dereference_array base types are not equal: ");
   ir->print();
   printf("\n");
               if (!ir->array_index->type->is_scalar()) {
      printf("ir_dereference_array @ %p does not have scalar index: %s\n",
                     if (!ir->array_index->type->is_integer_16_32()) {
      printf("ir_dereference_array @ %p does not have integer index: %s\n",
                        }
      ir_visitor_status
   ir_validate::visit_enter(class ir_dereference_record *ir)
   {
      if (!ir->record->type->is_struct() && !ir->record->type->is_interface()) {
      printf("ir_dereference_record @ %p does not specify a record\n",
         ir->print();
   printf("\n");
               if (ir->record->type->fields.structure[ir->field_idx].type != ir->type) {
      printf("ir_dereference_record type is not equal to the record "
         ir->print();
   printf("\n");
                  }
      ir_visitor_status
   ir_validate::visit_enter(ir_discard *ir)
   {
      if (ir->condition && ir->condition->type != glsl_type::bool_type) {
      printf("ir_discard condition %s type instead of bool.\n",
   glsl_get_type_name(ir->condition->type));
   ir->print();
   printf("\n");
                  }
      ir_visitor_status
   ir_validate::visit_enter(ir_if *ir)
   {
      if (ir->condition->type != glsl_type::bool_type) {
      printf("ir_if condition %s type instead of bool.\n",
   glsl_get_type_name(ir->condition->type));
   ir->print();
   printf("\n");
                  }
         ir_visitor_status
   ir_validate::visit_enter(ir_function *ir)
   {
      /* Function definitions cannot be nested.
   */
   if (this->current_function != NULL) {
      printf("Function definition nested inside another function "
   "definition:\n");
   printf("%s %p inside %s %p\n",
   ir->name, (void *) ir,
   this->current_function->name, (void *) this->current_function);
               /* Store the current function hierarchy being traversed.  This is used
   * by the function signature visitor to ensure that the signatures are
   * linked with the correct functions.
   */
                     /* Verify that all of the things stored in the list of signatures are,
   * in fact, function signatures.
   */
   foreach_in_list(ir_instruction, sig, &ir->signatures) {
      printf("Non-signature in signature list of function `%s'\n",
   ir->name);
   abort();
                        }
      ir_visitor_status
   ir_validate::visit_leave(ir_function *ir)
   {
               this->current_function = NULL;
      }
      ir_visitor_status
   ir_validate::visit_enter(ir_function_signature *ir)
   {
      if (this->current_function != ir->function()) {
      printf("Function signature nested inside wrong function "
   "definition:\n");
   printf("%p inside %s %p instead of %s %p\n",
   (void *) ir,
   this->current_function->name, (void *) this->current_function,
   ir->function_name(), (void *) ir->function());
               if (ir->return_type == NULL) {
      printf("Function signature %p for function %s has NULL return type.\n",
   (void *) ir, ir->function_name());
                           }
      ir_visitor_status
   ir_validate::visit_enter(ir_return *ir)
   {
      if (!this->current_function) {
      printf("Return statement outside of a function\n");
                  }
      ir_visitor_status
   ir_validate::visit_leave(ir_expression *ir)
   {
      for (unsigned i = ir->num_operands; i < 4; i++) {
                  for (unsigned i = 0; i < ir->num_operands; i++) {
                  switch (ir->operation) {
   case ir_unop_bit_not:
      assert(ir->operands[0]->type == ir->type);
      case ir_unop_logic_not:
      assert(ir->type->is_boolean());
   assert(ir->operands[0]->type->is_boolean());
         case ir_unop_neg:
      assert(ir->type == ir->operands[0]->type);
         case ir_unop_abs:
   case ir_unop_sign:
      assert(ir->operands[0]->type->is_int_16_32_64() ||
         assert(ir->type == ir->operands[0]->type);
         case ir_unop_rcp:
   case ir_unop_rsq:
   case ir_unop_sqrt:
      assert(ir->type->is_float_16_32_64());
   assert(ir->type == ir->operands[0]->type);
         case ir_unop_exp:
   case ir_unop_log:
   case ir_unop_exp2:
   case ir_unop_log2:
   case ir_unop_saturate:
      assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->type == ir->operands[0]->type);
         case ir_unop_f2i:
      assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->type->is_int_16_32());
      case ir_unop_f2u:
      assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->type->is_uint_16_32());
      case ir_unop_i2f:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->is_float_16_32());
      case ir_unop_f2b:
      assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->type->is_boolean());
      case ir_unop_f162b:
      assert(ir->operands[0]->type->base_type ==
         assert(ir->type->is_boolean());
      case ir_unop_b2f:
      assert(ir->operands[0]->type->is_boolean());
   assert(ir->type->is_float_16_32());
      case ir_unop_b2f16:
      assert(ir->operands[0]->type->is_boolean());
   assert(ir->type->base_type == GLSL_TYPE_FLOAT16);
      case ir_unop_i2b:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->is_boolean());
      case ir_unop_b2i:
      assert(ir->operands[0]->type->is_boolean());
   assert(ir->type->is_int_16_32());
      case ir_unop_u2f:
      assert(ir->operands[0]->type->is_uint_16_32());
   assert(ir->type->is_float_16_32());
      case ir_unop_i2u:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->is_uint_16_32());
      case ir_unop_u2i:
      assert(ir->operands[0]->type->is_uint_16_32());
   assert(ir->type->is_int_16_32());
      case ir_unop_bitcast_i2f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT);
   assert(ir->type->base_type == GLSL_TYPE_FLOAT);
      case ir_unop_bitcast_f2i:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
   assert(ir->type->base_type == GLSL_TYPE_INT);
      case ir_unop_bitcast_u2f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT);
   assert(ir->type->base_type == GLSL_TYPE_FLOAT);
      case ir_unop_bitcast_f2u:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
   assert(ir->type->base_type == GLSL_TYPE_UINT);
         case ir_unop_bitcast_u642d:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT64);
   assert(ir->type->is_double());
      case ir_unop_bitcast_i642d:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->is_double());
      case ir_unop_bitcast_d2u64:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->base_type == GLSL_TYPE_UINT64);
      case ir_unop_bitcast_d2i64:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_i642i:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->is_int_16_32());
      case ir_unop_u642i:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT64);
   assert(ir->type->is_int_16_32());
      case ir_unop_i642u:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->is_uint_16_32());
      case ir_unop_u642u:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT64);
   assert(ir->type->is_uint_16_32());
      case ir_unop_i642b:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->is_boolean());
      case ir_unop_i642f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->is_float());
      case ir_unop_u642f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT64);
   assert(ir->type->is_float());
      case ir_unop_i642d:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->is_double());
      case ir_unop_u642d:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT64);
   assert(ir->type->is_double());
      case ir_unop_i2i64:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_u2i64:
      assert(ir->operands[0]->type->is_uint_16_32());
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_b2i64:
      assert(ir->operands[0]->type->is_boolean());
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_f2i64:
      assert(ir->operands[0]->type->is_float());
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_d2i64:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_i2u64:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->base_type == GLSL_TYPE_UINT64);
      case ir_unop_u2u64:
      assert(ir->operands[0]->type->is_uint_16_32());
   assert(ir->type->base_type == GLSL_TYPE_UINT64);
      case ir_unop_f2u64:
      assert(ir->operands[0]->type->is_float());
   assert(ir->type->base_type == GLSL_TYPE_UINT64);
      case ir_unop_d2u64:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->base_type == GLSL_TYPE_UINT64);
      case ir_unop_u642i64:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT64);
   assert(ir->type->base_type == GLSL_TYPE_INT64);
      case ir_unop_i642u64:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT64);
   assert(ir->type->base_type == GLSL_TYPE_UINT64);
      case ir_unop_trunc:
   case ir_unop_round_even:
   case ir_unop_ceil:
   case ir_unop_floor:
   case ir_unop_fract:
      assert(ir->operands[0]->type->is_float_16_32_64());
   assert(ir->operands[0]->type == ir->type);
      case ir_unop_sin:
   case ir_unop_cos:
   case ir_unop_dFdx:
   case ir_unop_dFdx_coarse:
   case ir_unop_dFdx_fine:
   case ir_unop_dFdy:
   case ir_unop_dFdy_coarse:
   case ir_unop_dFdy_fine:
      assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->operands[0]->type == ir->type);
         case ir_unop_pack_snorm_2x16:
   case ir_unop_pack_unorm_2x16:
   case ir_unop_pack_half_2x16:
      assert(ir->type == glsl_type::uint_type);
   assert(ir->operands[0]->type == glsl_type::vec2_type);
         case ir_unop_pack_snorm_4x8:
   case ir_unop_pack_unorm_4x8:
      assert(ir->type == glsl_type::uint_type);
   assert(ir->operands[0]->type == glsl_type::vec4_type);
         case ir_unop_pack_double_2x32:
      assert(ir->type == glsl_type::double_type);
   assert(ir->operands[0]->type == glsl_type::uvec2_type);
         case ir_unop_pack_int_2x32:
      assert(ir->type == glsl_type::int64_t_type);
   assert(ir->operands[0]->type == glsl_type::ivec2_type);
         case ir_unop_pack_uint_2x32:
      assert(ir->type == glsl_type::uint64_t_type);
   assert(ir->operands[0]->type == glsl_type::uvec2_type);
         case ir_unop_pack_sampler_2x32:
      assert(ir->type->is_sampler());
   assert(ir->operands[0]->type == glsl_type::uvec2_type);
         case ir_unop_pack_image_2x32:
      assert(ir->type->is_image());
   assert(ir->operands[0]->type == glsl_type::uvec2_type);
         case ir_unop_unpack_snorm_2x16:
   case ir_unop_unpack_unorm_2x16:
   case ir_unop_unpack_half_2x16:
      assert(ir->type == glsl_type::vec2_type);
   assert(ir->operands[0]->type == glsl_type::uint_type);
         case ir_unop_unpack_snorm_4x8:
   case ir_unop_unpack_unorm_4x8:
      assert(ir->type == glsl_type::vec4_type);
   assert(ir->operands[0]->type == glsl_type::uint_type);
         case ir_unop_unpack_double_2x32:
      assert(ir->type == glsl_type::uvec2_type);
   assert(ir->operands[0]->type == glsl_type::double_type);
         case ir_unop_unpack_int_2x32:
      assert(ir->type == glsl_type::ivec2_type);
   assert(ir->operands[0]->type == glsl_type::int64_t_type);
         case ir_unop_unpack_uint_2x32:
      assert(ir->type == glsl_type::uvec2_type);
   assert(ir->operands[0]->type == glsl_type::uint64_t_type);
         case ir_unop_unpack_sampler_2x32:
      assert(ir->type == glsl_type::uvec2_type);
   assert(ir->operands[0]->type->is_sampler());
         case ir_unop_unpack_image_2x32:
      assert(ir->type == glsl_type::uvec2_type);
   assert(ir->operands[0]->type->is_image());
         case ir_unop_bitfield_reverse:
      assert(ir->operands[0]->type == ir->type);
   assert(ir->type->is_integer_32());
         case ir_unop_bit_count:
   case ir_unop_find_msb:
   case ir_unop_find_lsb:
      assert(ir->operands[0]->type->vector_elements == ir->type->vector_elements);
   assert(ir->operands[0]->type->is_integer_16_32());
   assert(ir->type->is_int_16_32());
         case ir_unop_clz:
      assert(ir->operands[0]->type == ir->type);
   assert(ir->type->is_uint_16_32());
         case ir_unop_interpolate_at_centroid:
      assert(ir->operands[0]->type == ir->type);
   assert(ir->operands[0]->type->is_float_16_32());
         case ir_unop_get_buffer_size:
      assert(ir->type == glsl_type::int_type);
   assert(ir->operands[0]->type == glsl_type::uint_type);
         case ir_unop_ssbo_unsized_array_length:
      assert(ir->type == glsl_type::int_type);
   assert(ir->operands[0]->type->is_array());
   assert(ir->operands[0]->type->is_unsized_array());
         case ir_unop_implicitly_sized_array_length:
      assert(ir->type == glsl_type::int_type);
   assert(ir->operands[0]->type->is_array());
         case ir_unop_d2f:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->is_float());
      case ir_unop_f2d:
      assert(ir->operands[0]->type->is_float());
   assert(ir->type->is_double());
      case ir_unop_f162f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT16);
   assert(ir->type->is_float());
      case ir_unop_f2f16:
   case ir_unop_f2fmp:
      assert(ir->operands[0]->type->is_float());
   assert(ir->type->base_type == GLSL_TYPE_FLOAT16);
      case ir_unop_i2i:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->is_int_16_32());
   assert(ir->type->base_type != ir->operands[0]->type->base_type);
      case ir_unop_u2u:
      assert(ir->operands[0]->type->is_uint_16_32());
   assert(ir->type->is_uint_16_32());
   assert(ir->type->base_type != ir->operands[0]->type->base_type);
      case ir_unop_i2imp:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT);
   assert(ir->type->base_type == GLSL_TYPE_INT16);
      case ir_unop_u2ump:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT);
   assert(ir->type->base_type == GLSL_TYPE_UINT16);
      case ir_unop_d2i:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->is_int_16_32());
      case ir_unop_i2d:
      assert(ir->operands[0]->type->is_int_16_32());
   assert(ir->type->is_double());
      case ir_unop_d2u:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->is_uint_16_32());
      case ir_unop_u2d:
      assert(ir->operands[0]->type->is_uint_16_32());
   assert(ir->type->is_double());
      case ir_unop_d2b:
      assert(ir->operands[0]->type->is_double());
   assert(ir->type->is_boolean());
         case ir_unop_frexp_sig:
      assert(ir->operands[0]->type->is_float_32_64());
      case ir_unop_frexp_exp:
      assert(ir->operands[0]->type->is_float_32_64());
   assert(ir->type->base_type == GLSL_TYPE_INT);
      case ir_unop_subroutine_to_int:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_SUBROUTINE);
   assert(ir->type->base_type == GLSL_TYPE_INT);
         case ir_unop_atan:
      assert(ir->operands[0]->type->is_float_16_32_64());
   assert(ir->type == ir->operands[0]->type);
         case ir_binop_add:
   case ir_binop_sub:
   case ir_binop_mul:
   case ir_binop_div:
   case ir_binop_mod:
   case ir_binop_min:
   case ir_binop_max:
   case ir_binop_pow:
      assert(ir->operands[0]->type->base_type ==
            if (ir->operation == ir_binop_mul &&
      (ir->type->base_type == GLSL_TYPE_UINT64 ||
   ir->type->base_type == GLSL_TYPE_INT64) &&
   (ir->operands[0]->type->is_int_16_32()||
   ir->operands[1]->type->is_int_16_32()||
   ir->operands[0]->type->is_uint_16_32() ||
   ir->operands[1]->type->is_uint_16_32())) {
   assert(ir->operands[0]->type == ir->operands[1]->type);
               assert(ir->operands[1]->type == ir->type);
         assert(ir->operands[0]->type == ir->type);
         else if (ir->operands[0]->type->is_vector() &&
   assert(ir->operands[0]->type == ir->operands[1]->type);
   assert(ir->operands[0]->type == ir->type);
         }
         case ir_binop_abs_sub:
      assert(ir->operands[0]->type == ir->operands[1]->type);
   assert(ir->operands[0]->type->is_integer_16_32_64());
   assert(ir->operands[0]->type->vector_elements ==
         assert(ir->type->is_uint_16_32_64());
         case ir_binop_add_sat:
   case ir_binop_sub_sat:
   case ir_binop_avg:
   case ir_binop_avg_round:
      assert(ir->type == ir->operands[0]->type);
   assert(ir->type == ir->operands[1]->type);
   assert(ir->type->is_integer_16_32_64());
         case ir_binop_mul_32x16:
   case ir_binop_imul_high:
      assert(ir->type == ir->operands[0]->type);
   assert(ir->type == ir->operands[1]->type);
   assert(ir->type->is_integer_32());
         case ir_binop_carry:
   case ir_binop_borrow:
      assert(ir->type == ir->operands[0]->type);
   assert(ir->type == ir->operands[1]->type);
   assert(ir->type->base_type == GLSL_TYPE_UINT);
         case ir_binop_less:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_nequal:
      /* The semantics of the IR operators differ from the GLSL <, >, <=, >=,
   * ==, and != operators.  The IR operators perform a component-wise
   * comparison on scalar or vector types and return a boolean scalar or
   * vector type of the same size.
   */
   assert(ir->type->is_boolean());
   assert(ir->operands[0]->type == ir->operands[1]->type);
   assert(ir->operands[0]->type->is_vector()
   || ir->operands[0]->type->is_scalar());
   assert(ir->operands[0]->type->vector_elements
   == ir->type->vector_elements);
         case ir_binop_all_equal:
   case ir_binop_any_nequal:
      /* GLSL == and != operate on scalars, vectors, matrices and arrays, and
   * return a scalar boolean.  The IR matches that.
   */
   assert(ir->type == glsl_type::bool_type);
   assert(ir->operands[0]->type == ir->operands[1]->type);
         case ir_binop_lshift:
   case ir_binop_rshift:
      assert(ir->operands[0]->type->is_integer_16_32_64() &&
         if (ir->operands[0]->type->is_scalar()) {
         }
   if (ir->operands[0]->type->is_vector() &&
      ir->operands[1]->type->is_vector()) {
   assert(ir->operands[0]->type->components() ==
      }
   assert(ir->type == ir->operands[0]->type);
         case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
      assert(ir->operands[0]->type->base_type ==
         assert(ir->type->is_integer_16_32_64());
   if (ir->operands[0]->type->is_vector() &&
      ir->operands[1]->type->is_vector()) {
   assert(ir->operands[0]->type->vector_elements ==
      }
         case ir_binop_logic_and:
   case ir_binop_logic_xor:
   case ir_binop_logic_or:
      assert(ir->type->is_boolean());
   assert(ir->operands[0]->type->is_boolean());
   assert(ir->operands[1]->type->is_boolean());
         case ir_binop_dot:
      assert(ir->type == glsl_type::float_type ||
         ir->type == glsl_type::double_type ||
   assert(ir->operands[0]->type->is_float_16_32_64());
   assert(ir->operands[0]->type->is_vector());
   assert(ir->operands[0]->type == ir->operands[1]->type);
         case ir_binop_ubo_load:
               assert(ir->operands[1]->type == glsl_type::uint_type);
         case ir_binop_ldexp:
      assert(ir->operands[0]->type == ir->type);
   assert(ir->operands[0]->type->is_float_32_64());
   assert(ir->operands[1]->type->base_type == GLSL_TYPE_INT);
   assert(ir->operands[0]->type->components() ==
               case ir_binop_vector_extract:
      assert(ir->operands[0]->type->is_vector());
   assert(ir->operands[1]->type->is_scalar()
               case ir_binop_interpolate_at_offset:
      assert(ir->operands[0]->type == ir->type);
   assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->operands[1]->type->components() == 2);
   assert(ir->operands[1]->type->is_float_16_32());
         case ir_binop_interpolate_at_sample:
      assert(ir->operands[0]->type == ir->type);
   assert(ir->operands[0]->type->is_float_16_32());
   assert(ir->operands[1]->type == glsl_type::int_type ||
               case ir_binop_atan2:
      assert(ir->operands[0]->type->is_float_16_32_64());
   assert(ir->operands[1]->type == ir->operands[0]->type);
   assert(ir->type == ir->operands[0]->type);
         case ir_triop_fma:
      assert(ir->type->is_float_16_32_64());
   assert(ir->type == ir->operands[0]->type);
   assert(ir->type == ir->operands[1]->type);
   assert(ir->type == ir->operands[2]->type);
         case ir_triop_lrp:
      assert(ir->operands[0]->type->is_float_16_32_64());
   assert(ir->operands[0]->type == ir->operands[1]->type);
   assert(ir->operands[2]->type == ir->operands[0]->type ||
         ir->operands[2]->type == glsl_type::float_type ||
   ir->operands[2]->type == glsl_type::double_type ||
         case ir_triop_csel:
      assert(ir->operands[0]->type->is_boolean());
   assert(ir->type->vector_elements == ir->operands[0]->type->vector_elements);
   assert(ir->type == ir->operands[1]->type);
   assert(ir->type == ir->operands[2]->type);
         case ir_triop_bitfield_extract:
      assert(ir->type->is_integer_16_32());
   assert(ir->operands[0]->type == ir->type);
   assert(ir->operands[1]->type == ir->type);
   assert(ir->operands[2]->type == ir->type);
         case ir_triop_vector_insert:
      assert(ir->operands[0]->type->is_vector());
   assert(ir->operands[1]->type->is_scalar());
   assert(ir->operands[0]->type->base_type == ir->operands[1]->type->base_type);
   assert(ir->operands[2]->type->is_scalar()
         assert(ir->type == ir->operands[0]->type);
         case ir_quadop_bitfield_insert:
      assert(ir->type->is_integer_16_32());
   assert(ir->operands[0]->type == ir->type);
   assert(ir->operands[1]->type == ir->type);
   assert(ir->operands[2]->type == ir->type);
   assert(ir->operands[3]->type == ir->type);
         case ir_quadop_vector:
      /* The vector operator collects some number of scalars and generates a
   * vector from them.
   *
   *  - All of the operands must be scalar.
   *  - Number of operands must matche the size of the resulting vector.
   *  - Base type of the operands must match the base type of the result.
   */
   switch (ir->type->vector_elements) {
   case 1:
      assert(ir->operands[0]->type->is_scalar());
   assert(ir->operands[0]->type->base_type == ir->type->base_type);
   assert(ir->operands[1] == NULL);
   assert(ir->operands[2] == NULL);
   assert(ir->operands[3] == NULL);
      assert(ir->operands[0]->type->is_scalar());
   assert(ir->operands[0]->type->base_type == ir->type->base_type);
   assert(ir->operands[1]->type->is_scalar());
   assert(ir->operands[1]->type->base_type == ir->type->base_type);
   assert(ir->operands[2] == NULL);
   assert(ir->operands[3] == NULL);
   break;
         assert(ir->operands[0]->type->is_scalar());
   assert(ir->operands[0]->type->base_type == ir->type->base_type);
   assert(ir->operands[1]->type->is_scalar());
   assert(ir->operands[1]->type->base_type == ir->type->base_type);
   assert(ir->operands[2]->type->is_scalar());
   assert(ir->operands[2]->type->base_type == ir->type->base_type);
   assert(ir->operands[3] == NULL);
   break;
         assert(ir->operands[0]->type->is_scalar());
   assert(ir->operands[0]->type->base_type == ir->type->base_type);
   assert(ir->operands[1]->type->is_scalar());
   assert(ir->operands[1]->type->base_type == ir->type->base_type);
   assert(ir->operands[2]->type->is_scalar());
   assert(ir->operands[2]->type->base_type == ir->type->base_type);
   assert(ir->operands[3]->type->is_scalar());
   assert(ir->operands[3]->type->base_type == ir->type->base_type);
   break;
         /* The is_vector assertion above should prevent execution from ever
      * getting here.
      assert(!"Should not get here.");
   break;
                        }
      ir_visitor_status
   ir_validate::visit_leave(ir_swizzle *ir)
   {
               for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      printf("ir_swizzle @ %p specifies a channel not present "
   "in the value.\n", (void *) ir);
   ir->print();
   abort();
                        }
      ir_visitor_status
   ir_validate::visit(ir_variable *ir)
   {
      /* An ir_variable is the one thing that can (and will) appear multiple times
   * in an IR tree.  It is added to the hashtable so that it can be used
   * in the ir_dereference_variable handler to ensure that a variable is
   * declared before it is dereferenced.
   */
   if (ir->name && ir->is_name_ralloced())
                     /* If a variable is an array, verify that the maximum array index is in
   * bounds.  There was once an error in AST-to-HIR conversion that set this
   * to be out of bounds.
   */
   if (ir->type->array_size() > 0) {
      printf("ir_variable has maximum access out of bounds (%d vs %d)\n",
   ir->data.max_array_access, ir->type->length - 1);
   ir->print();
   abort();
                     /* If a variable is an interface block (or an array of interface blocks),
   * verify that the maximum array index for each interface member is in
   * bounds.
   */
   if (ir->is_interface_instance()) {
      const glsl_struct_field *fields =
         for (unsigned i = 0; i < ir->get_interface_type()->length; i++) {
      if (fields[i].type->array_size() > 0 &&
      !fields[i].implicit_sized_array) {
                           if (max_ifc_array_access[i] >= (int)fields[i].type->length) {
      printf("ir_variable has maximum access out of bounds for "
         "field %s (%d vs %d)\n", fields[i].name,
   ir->print();
                        if (ir->constant_initializer != NULL && !ir->data.has_initializer) {
      printf("ir_variable didn't have an initializer, but has a constant "
   "initializer value.\n");
   ir->print();
               if (ir->data.mode == ir_var_uniform
      && is_gl_identifier(ir->name)
   && ir->get_state_slots() == NULL) {
   printf("built-in uniform has no state\n");
   ir->print();
                  }
      ir_visitor_status
   ir_validate::visit_enter(ir_assignment *ir)
   {
      const ir_dereference *const lhs = ir->lhs;
   if (lhs->type->is_scalar() || lhs->type->is_vector()) {
      printf("Assignment LHS is %s, but write mask is 0:\n",
   lhs->type->is_scalar() ? "scalar" : "vector");
   ir->print();
   abort();
                  int lhs_components = 0;
   if (ir->write_mask & (1 << i))
      lhs_components++;
               printf("Assignment count of LHS write mask channels enabled not\n"
   "matching RHS vector size (%d LHS, %d RHS).\n",
   lhs_components, ir->rhs->type->vector_elements);
   ir->print();
   abort();
                     if (lhs->type->base_type != ir->rhs->type->base_type) {
      printf("Assignment LHS and RHS base types are different:\n");
   lhs->print();
   printf("\n");
   ir->rhs->print();
   printf("\n");
                           }
      ir_visitor_status
   ir_validate::visit_enter(ir_call *ir)
   {
               if (callee->ir_type != ir_type_function_signature) {
      printf("IR called by ir_call is not ir_function_signature!\n");
               if (ir->return_deref) {
      printf("callee type %s does not match return storage type %s\n",
         abort();
            } else if (callee->return_type != glsl_type::void_type) {
      printf("ir_call has non-void callee but no return storage\n");
               const exec_node *formal_param_node = callee->parameters.get_head_raw();
   const exec_node *actual_param_node = ir->actual_parameters.get_head_raw();
   while (true) {
      if (formal_param_node->is_tail_sentinel()
      != actual_param_node->is_tail_sentinel()) {
   printf("ir_call has the wrong number of parameters:\n");
      }
   if (formal_param_node->is_tail_sentinel()) {
         }
   const ir_variable *formal_param
         const ir_rvalue *actual_param
         if (formal_param->type != actual_param->type) {
      printf("ir_call parameter type mismatch:\n");
      }
   if (formal_param->data.mode == ir_var_function_out
      || formal_param->data.mode == ir_var_function_inout) {
   if (!actual_param->is_lvalue()) {
      printf("ir_call out/inout parameters must be lvalues:\n");
         }
   formal_param_node = formal_param_node->next;
                     dump_ir:
      ir->print();
   printf("callee:\n");
   callee->print();
   abort();
      }
      void
   ir_validate::validate_ir(ir_instruction *ir, void *data)
   {
               if (_mesa_set_search(ir_set, ir)) {
      printf("Instruction node present twice in ir tree:\n");
   ir->print();
   printf("\n");
      }
      }
      static void
   check_node_type(ir_instruction *ir, void *data)
   {
               if (ir->ir_type >= ir_type_max) {
      printf("Instruction node with unset type\n");
      }
   ir_rvalue *value = ir->as_rvalue();
   if (value != NULL)
      }
      void
   validate_ir_tree(exec_list *instructions)
   {
      /* We shouldn't have any reason to validate IR in a release build,
   * and it's half composed of assert()s anyway which wouldn't do
   * anything.
      #ifndef DEBUG
      if (!debug_get_bool_option("GLSL_VALIDATE", false))
      #endif
                        foreach_in_list(ir_instruction, ir, instructions) {
            }
