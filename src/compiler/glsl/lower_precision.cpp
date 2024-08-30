   /*
   * Copyright © 2019 Google, Inc
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
   * \file lower_precision.cpp
   */
      #include "main/macros.h"
   #include "main/consts_exts.h"
   #include "compiler/glsl_types.h"
   #include "ir.h"
   #include "ir_builder.h"
   #include "ir_optimization.h"
   #include "ir_rvalue_visitor.h"
   #include "util/half_float.h"
   #include "util/set.h"
   #include "util/hash_table.h"
   #include <vector>
      namespace {
      class find_precision_visitor : public ir_rvalue_enter_visitor {
   public:
      find_precision_visitor(const struct gl_shader_compiler_options *options);
            virtual void handle_rvalue(ir_rvalue **rvalue);
                     /* Set of rvalues that can be lowered. This will be filled in by
   * find_lowerable_rvalues_visitor. Only the root node of a lowerable section
   * will be added to this set.
   */
            /**
   * A mapping of builtin signature functions to lowered versions. This is
   * filled in lazily when a lowered version is needed.
   */
   struct hash_table *lowered_builtins;
   /**
   * A temporary hash table only used in order to clone functions.
   */
                        };
      class find_lowerable_rvalues_visitor : public ir_hierarchical_visitor {
   public:
      enum can_lower_state {
      UNKNOWN,
   CANT_LOWER,
               enum parent_relation {
      /* The parent performs a further operation involving the result from the
   * child and can be lowered along with it.
   */
   COMBINED_OPERATION,
   /* The parent instruction’s operation is independent of the child type so
   * the child should be lowered separately.
   */
               struct stack_entry {
      ir_instruction *instr;
   enum can_lower_state state;
   /* List of child rvalues that can be lowered. When this stack entry is
   * popped, if this node itself can’t be lowered than all of the children
   * are root nodes to lower so we will add them to lowerable_rvalues.
   * Otherwise if this node can also be lowered then we won’t add the
   * children because we only want to add the topmost lowerable nodes to
   * lowerable_rvalues and the children will be lowered as part of lowering
   * this node.
   */
               find_lowerable_rvalues_visitor(struct set *result,
            static void stack_enter(class ir_instruction *ir, void *data);
            virtual ir_visitor_status visit(ir_constant *ir);
            virtual ir_visitor_status visit_enter(ir_dereference_record *ir);
   virtual ir_visitor_status visit_enter(ir_dereference_array *ir);
   virtual ir_visitor_status visit_enter(ir_texture *ir);
            virtual ir_visitor_status visit_leave(ir_assignment *ir);
            can_lower_state handle_precision(const glsl_type *type,
            static parent_relation get_parent_relation(ir_instruction *parent,
            std::vector<stack_entry> stack;
   struct set *lowerable_rvalues;
            void pop_stack_entry();
      };
      class lower_precision_visitor : public ir_rvalue_visitor {
   public:
      virtual void handle_rvalue(ir_rvalue **rvalue);
   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit_enter(ir_dereference_record *);
   virtual ir_visitor_status visit_enter(ir_call *ir);
   virtual ir_visitor_status visit_enter(ir_texture *ir);
      };
      static bool
   can_lower_type(const struct gl_shader_compiler_options *options,
         {
      /* Don’t lower any expressions involving non-float types except bool and
   * texture samplers. This will rule out operations that change the type such
   * as conversion to ints. Instead it will end up lowering the arguments
   * instead and adding a final conversion to float32. We want to handle
   * boolean types so that it will do comparisons as 16-bit.
            switch (type->without_array()->base_type) {
   /* TODO: should we do anything for these two with regard to Int16 vs FP16
   * support?
   */
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_IMAGE:
            case GLSL_TYPE_FLOAT:
            case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
            default:
            }
      find_lowerable_rvalues_visitor::find_lowerable_rvalues_visitor(struct set *res,
         {
      lowerable_rvalues = res;
   options = opts;
   callback_enter = stack_enter;
   callback_leave = stack_leave;
   data_enter = this;
      }
      void
   find_lowerable_rvalues_visitor::stack_enter(class ir_instruction *ir,
         {
      find_lowerable_rvalues_visitor *state =
            /* Add a new stack entry for this instruction */
            entry.instr = ir;
               }
      void
   find_lowerable_rvalues_visitor::add_lowerable_children(const stack_entry &entry)
   {
      /* We can’t lower this node so if there were any pending children then they
   * are all root lowerable nodes and we should add them to the set.
   */
   for (auto &it : entry.lowerable_children)
      }
      void
   find_lowerable_rvalues_visitor::pop_stack_entry()
   {
               if (stack.size() >= 2) {
      /* Combine this state into the parent state, unless the parent operation
   * doesn’t have any relation to the child operations
   */
   stack_entry &parent = stack.end()[-2];
            if (rel == COMBINED_OPERATION) {
      switch (entry.state) {
   case CANT_LOWER:
      parent.state = CANT_LOWER;
      case SHOULD_LOWER:
      if (parent.state == UNKNOWN)
            case UNKNOWN:
                        if (entry.state == SHOULD_LOWER) {
               if (rv == NULL) {
         } else if (stack.size() >= 2) {
               switch (get_parent_relation(parent.instr, rv)) {
   case COMBINED_OPERATION:
      /* We only want to add the toplevel lowerable instructions to the
   * lowerable set. Therefore if there is a parent then instead of
   * adding this instruction to the set we will queue depending on
   * the result of the parent instruction.
   */
   parent.lowerable_children.push_back(entry.instr);
      case INDEPENDENT_OPERATION:
      _mesa_set_add(lowerable_rvalues, rv);
         } else {
      /* This is a toplevel node so add it directly to the lowerable
   * set.
   */
         } else if (entry.state == CANT_LOWER) {
                     }
      void
   find_lowerable_rvalues_visitor::stack_leave(class ir_instruction *ir,
         {
      find_lowerable_rvalues_visitor *state =
               }
      enum find_lowerable_rvalues_visitor::can_lower_state
   find_lowerable_rvalues_visitor::handle_precision(const glsl_type *type,
         {
      if (!can_lower_type(options, type))
            switch (precision) {
   case GLSL_PRECISION_NONE:
         case GLSL_PRECISION_HIGH:
         case GLSL_PRECISION_MEDIUM:
   case GLSL_PRECISION_LOW:
                     }
      enum find_lowerable_rvalues_visitor::parent_relation
   find_lowerable_rvalues_visitor::get_parent_relation(ir_instruction *parent,
         {
      /* If the parent is a dereference instruction then the only child could be
   * for example an array dereference and that should be lowered independently
   * of the parent.
   */
   if (parent->as_dereference())
            /* The precision of texture sampling depend on the precision of the sampler.
   * The rest of the arguments don’t matter so we can treat it as an
   * independent operation.
   */
   if (parent->as_texture())
               }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit(ir_constant *ir)
   {
               if (!can_lower_type(options, ir->type))
                        }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit(ir_dereference_variable *ir)
   {
               if (stack.back().state == UNKNOWN)
                        }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit_enter(ir_dereference_record *ir)
   {
               if (stack.back().state == UNKNOWN)
               }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit_enter(ir_dereference_array *ir)
   {
               if (stack.back().state == UNKNOWN)
               }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit_enter(ir_texture *ir)
   {
               /* The precision of the sample value depends on the precision of the
   * sampler.
   */
   stack.back().state = handle_precision(ir->type,
            }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit_enter(ir_expression *ir)
   {
               if (!can_lower_type(options, ir->type))
            /* Don't lower precision for derivative calculations */
   if (!options->LowerPrecisionDerivatives &&
      (ir->operation == ir_unop_dFdx ||
   ir->operation == ir_unop_dFdx_coarse ||
   ir->operation == ir_unop_dFdx_fine ||
   ir->operation == ir_unop_dFdy ||
   ir->operation == ir_unop_dFdy_coarse ||
   ir->operation == ir_unop_dFdy_fine)) {
                  }
      static unsigned
   handle_call(ir_call *ir, const struct set *lowerable_rvalues)
   {
      /* The intrinsic call is inside the wrapper imageLoad function that will
   * be inlined. We have to handle both of them.
   */
   if (ir->callee->intrinsic_id == ir_intrinsic_image_load ||
      (ir->callee->is_builtin() &&
   !strcmp(ir->callee_name(), "imageLoad"))) {
   ir_rvalue *param = (ir_rvalue*)ir->actual_parameters.get_head();
            assert(ir->callee->return_precision == GLSL_PRECISION_HIGH);
            /* GLSL ES 3.20 requires that images have a precision modifier, but if
   * you set one, it doesn't do anything, because all intrinsics are
   * defined with highp. This seems to be a spec bug.
   *
   * In theory we could set the return value to mediump if the image
   * format has a lower precision. This appears to be the most sensible
   * thing to do.
   */
   const struct util_format_description *desc =
         int i =
                           if (desc->channel[i].pure_integer ||
      desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT)
      else
                        /* Return the declared precision for user-defined functions. */
   if (!ir->callee->is_builtin() || ir->callee->return_precision != GLSL_PRECISION_NONE)
            /* Handle special calls. */
   if (ir->callee->is_builtin() && ir->actual_parameters.length()) {
      ir_rvalue *param = (ir_rvalue*)ir->actual_parameters.get_head();
            /* Handle builtin wrappers around ir_texture opcodes. These wrappers will
   * be inlined by lower_precision() if we return true here, so that we can
   * get to ir_texture later and do proper lowering.
   *
   * We should lower the type of the return value if the sampler type
   * uses lower precision. The function parameters don't matter.
   */
   if (var && var->type->without_array()->is_sampler()) {
      /* textureGatherOffsets always takes a highp array of constants. As
   * per the discussion https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/16547#note_1393704
   * trying to lower the precision results in segfault later on
   * in the compiler as textureGatherOffsets will end up being passed
   * a temp when its expecting a constant as required by the spec.
   */
                                 if (ir->callee->return_precision != GLSL_PRECISION_NONE)
            if (/* Parameters are always implicitly promoted to highp: */
      !strcmp(ir->callee_name(), "floatBitsToInt") ||
   !strcmp(ir->callee_name(), "floatBitsToUint") ||
   !strcmp(ir->callee_name(), "intBitsToFloat") ||
   !strcmp(ir->callee_name(), "uintBitsToFloat"))
         /* Number of parameters to check if they are lowerable. */
            /* "For the interpolateAt* functions, the call will return a precision
   *  qualification matching the precision of the interpolant argument to the
   *  function call."
   *
   * and
   *
   * "The precision qualification of the value returned from bitfieldExtract()
   *  matches the precision qualification of the call's input argument
   *  “value”."
   */
   if (!strcmp(ir->callee_name(), "interpolateAtOffset") ||
      !strcmp(ir->callee_name(), "interpolateAtSample") ||
   !strcmp(ir->callee_name(), "bitfieldExtract")) {
      } else if (!strcmp(ir->callee_name(), "bitfieldInsert")) {
      /* "The precision qualification of the value returned from bitfieldInsert
   * matches the highest precision qualification of the call's input
   * arguments “base” and “insert”."
   */
               /* If the call is to a builtin, then the function won’t have a return
   * precision and we should determine it from the precision of the arguments.
   */
   foreach_in_list(ir_rvalue, param, &ir->actual_parameters) {
      if (!check_parameters)
            if (!param->as_constant() &&
                                 }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit_leave(ir_call *ir)
   {
               /* Special case for handling temporary variables generated by the compiler
   * for function calls. If we assign to one of these using a function call
   * that has a lowerable return type then we can assume the temporary
   * variable should have a medium precision too.
            /* Do nothing if the return type is void. */
   if (!ir->return_deref)
                                       can_lower_state lower_state =
            if (lower_state == SHOULD_LOWER) {
      /* Function calls always write to a temporary return value in the caller,
   * which has no other users.  That temp may start with the precision of
   * the function's signature, but if we're inferring the precision of an
   * unqualified builtin operation (particularly the imageLoad overrides!)
   * then we need to update it.
   */
      } else {
                     }
      ir_visitor_status
   find_lowerable_rvalues_visitor::visit_leave(ir_assignment *ir)
   {
               /* Special case for handling temporary variables generated by the compiler.
   * If we assign to one of these using a lowered precision then we can assume
   * the temporary variable should have a medium precision too.
   */
            if (var->data.mode == ir_var_temporary) {
      if (_mesa_set_search(lowerable_rvalues, ir->rhs)) {
      /* Only override the precision if this is the first assignment. For
   * temporaries such as the ones generated for the ?: operator there
   * can be multiple assignments with different precisions. This way we
   * get the highest precision of all of the assignments.
   */
   if (var->data.precision == GLSL_PRECISION_NONE)
      } else if (!ir->rhs->as_constant()) {
                        }
      void
   find_lowerable_rvalues(const struct gl_shader_compiler_options *options,
               {
                           }
      static const glsl_type *
   convert_type(bool up, const glsl_type *type)
   {
      if (type->is_array()) {
      return glsl_type::get_array_instance(convert_type(up, type->fields.array),
                              if (up) {
      switch (type->base_type) {
   case GLSL_TYPE_FLOAT16:
      new_base_type = GLSL_TYPE_FLOAT;
      case GLSL_TYPE_INT16:
      new_base_type = GLSL_TYPE_INT;
      case GLSL_TYPE_UINT16:
      new_base_type = GLSL_TYPE_UINT;
      default:
      unreachable("invalid type");
         } else {
      switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      new_base_type = GLSL_TYPE_FLOAT16;
      case GLSL_TYPE_INT:
      new_base_type = GLSL_TYPE_INT16;
      case GLSL_TYPE_UINT:
      new_base_type = GLSL_TYPE_UINT16;
      default:
      unreachable("invalid type");
                  return glsl_type::get_instance(new_base_type,
                        }
      static const glsl_type *
   lower_glsl_type(const glsl_type *type)
   {
         }
      static ir_rvalue *
   convert_precision(bool up, ir_rvalue *ir)
   {
               if (up) {
      switch (ir->type->base_type) {
   case GLSL_TYPE_FLOAT16:
      op = ir_unop_f162f;
      case GLSL_TYPE_INT16:
      op = ir_unop_i2i;
      case GLSL_TYPE_UINT16:
      op = ir_unop_u2u;
      default:
      unreachable("invalid type");
         } else {
      switch (ir->type->base_type) {
   case GLSL_TYPE_FLOAT:
      op = ir_unop_f2fmp;
      case GLSL_TYPE_INT:
      op = ir_unop_i2imp;
      case GLSL_TYPE_UINT:
      op = ir_unop_u2ump;
      default:
      unreachable("invalid type");
                  const glsl_type *desired_type = convert_type(up, ir->type);
   void *mem_ctx = ralloc_parent(ir);
      }
      void
   lower_precision_visitor::handle_rvalue(ir_rvalue **rvalue)
   {
               if (ir == NULL)
            if (ir->as_dereference()) {
      if (!ir->type->is_boolean())
      } else if (ir->type->is_32bit()) {
                        if (const_ir) {
               if (ir->type->base_type == GLSL_TYPE_FLOAT16) {
      for (unsigned i = 0; i < ARRAY_SIZE(value.f16); i++)
      } else if (ir->type->base_type == GLSL_TYPE_INT16) {
      for (unsigned i = 0; i < ARRAY_SIZE(value.i16); i++)
      } else if (ir->type->base_type == GLSL_TYPE_UINT16) {
      for (unsigned i = 0; i < ARRAY_SIZE(value.u16); i++)
      } else {
                           }
      ir_visitor_status
   lower_precision_visitor::visit_enter(ir_dereference_record *ir)
   {
      /* We don’t want to lower the variable */
      }
      ir_visitor_status
   lower_precision_visitor::visit_enter(ir_dereference_array *ir)
   {
      /* We don’t want to convert the array index or the variable. If the array
   * index itself is lowerable that will be handled separately.
   */
      }
      ir_visitor_status
   lower_precision_visitor::visit_enter(ir_call *ir)
   {
      /* We don’t want to convert the arguments. These will be handled separately.
   */
      }
      ir_visitor_status
   lower_precision_visitor::visit_enter(ir_texture *ir)
   {
      /* We don’t want to convert the arguments. These will be handled separately.
   */
      }
      ir_visitor_status
   lower_precision_visitor::visit_leave(ir_expression *ir)
   {
               /* If the expression is a conversion operation to or from bool then fix the
   * operation.
   */
   switch (ir->operation) {
   case ir_unop_b2f:
      ir->operation = ir_unop_b2f16;
      case ir_unop_f2b:
      ir->operation = ir_unop_f162b;
      case ir_unop_b2i:
   case ir_unop_i2b:
      /* Nothing to do - they both support int16. */
      default:
                     }
      void
   find_precision_visitor::handle_rvalue(ir_rvalue **rvalue)
   {
      /* Checking the precision of rvalue can be lowered first throughout
   * find_lowerable_rvalues_visitor.
   * Once it found the precision of rvalue can be lowered, then we can
   * add conversion f2fmp, etc. through lower_precision_visitor.
   */
   if (*rvalue == NULL)
                     if (!entry)
                     /* If the entire expression is just a variable dereference then trying to
   * lower it will just directly add pointless to and from conversions without
   * any actual operation in-between. Although these will eventually get
   * optimised out, avoiding generating them here also avoids breaking inout
   * parameters to functions.
   */
   if ((*rvalue)->as_dereference())
                     (*rvalue)->accept(&v);
            /* We don’t need to add the final conversion if the final type has been
   * converted to bool
   */
   if ((*rvalue)->type->base_type != GLSL_TYPE_BOOL) {
            }
      ir_visitor_status
   find_precision_visitor::visit_enter(ir_call *ir)
   {
               ir_variable *return_var =
            /* Don't do anything for image_load here. We have only changed the return
   * value to mediump/lowp, so that following instructions can use reduced
   * precision.
   *
   * The return value type of the intrinsic itself isn't changed here, but
   * can be changed in NIR if all users use the *2*mp opcode.
   */
   if (ir->callee->intrinsic_id == ir_intrinsic_image_load)
            /* If this is a call to a builtin and the find_lowerable_rvalues_visitor
   * overrode the precision of the temporary return variable, then we can
   * replace the builtin implementation with a lowered version.
            if (!ir->callee->is_builtin() ||
      ir->callee->is_intrinsic() ||
   return_var == NULL ||
   (return_var->data.precision != GLSL_PRECISION_MEDIUM &&
   return_var->data.precision != GLSL_PRECISION_LOW))
         ir->callee = map_builtin(ir->callee);
   ir->generate_inline(ir);
               }
      ir_function_signature *
   find_precision_visitor::map_builtin(ir_function_signature *sig)
   {
      if (lowered_builtins == NULL) {
      lowered_builtins = _mesa_pointer_hash_table_create(NULL);
   clone_ht =_mesa_pointer_hash_table_create(NULL);
      } else {
      struct hash_entry *entry = _mesa_hash_table_search(lowered_builtins, sig);
   if (entry)
               ir_function_signature *lowered_sig =
            /* If we're lowering the output precision of the function, then also lower
   * the precision of its inputs unless they have a specific qualifier.  The
   * exception is bitCount, which doesn't declare its arguments highp but
   * should not be lowering the args to mediump just because the output is
   * lowp.
   */
   if (strcmp(sig->function_name(), "bitCount") != 0) {
      foreach_in_list(ir_variable, param, &lowered_sig->parameters) {
      /* Demote the precision of unqualified function arguments. */
   if (param->data.precision == GLSL_PRECISION_NONE)
                                                }
      find_precision_visitor::find_precision_visitor(const struct gl_shader_compiler_options *options)
      : lowerable_rvalues(_mesa_pointer_set_create(NULL)),
   lowered_builtins(NULL),
   clone_ht(NULL),
   lowered_builtin_mem_ctx(NULL),
      {
   }
      find_precision_visitor::~find_precision_visitor()
   {
               if (lowered_builtins) {
      _mesa_hash_table_destroy(lowered_builtins, NULL);
   _mesa_hash_table_destroy(clone_ht, NULL);
         }
      /* Lowering opcodes to 16 bits is not enough for programs with control flow
   * (and the ?: operator, which is represented by if-then-else in the IR),
   * because temporary variables, which are used for passing values between
   * code blocks, are not lowered, resulting in 32-bit phis in NIR.
   *
   * First change the variable types to 16 bits, then change all ir_dereference
   * types to 16 bits.
   */
   class lower_variables_visitor : public ir_rvalue_enter_visitor {
   public:
      lower_variables_visitor(const struct gl_shader_compiler_options *options)
      : options(options) {
               virtual ~lower_variables_visitor()
   {
                  virtual ir_visitor_status visit(ir_variable *var);
   virtual ir_visitor_status visit_enter(ir_assignment *ir);
   virtual ir_visitor_status visit_enter(ir_return *ir);
   virtual ir_visitor_status visit_enter(ir_call *ir);
            void fix_types_in_deref_chain(ir_dereference *ir);
   void convert_split_assignment(ir_dereference *lhs, ir_rvalue *rhs,
            const struct gl_shader_compiler_options *options;
      };
      static void
   lower_constant(ir_constant *ir)
   {
      if (ir->type->is_array()) {
      for (int i = 0; i < ir->type->array_size(); i++)
            ir->type = lower_glsl_type(ir->type);
               ir->type = lower_glsl_type(ir->type);
            if (ir->type->base_type == GLSL_TYPE_FLOAT16) {
      for (unsigned i = 0; i < ARRAY_SIZE(value.f16); i++)
      } else if (ir->type->base_type == GLSL_TYPE_INT16) {
      for (unsigned i = 0; i < ARRAY_SIZE(value.i16); i++)
      } else if (ir->type->base_type == GLSL_TYPE_UINT16) {
      for (unsigned i = 0; i < ARRAY_SIZE(value.u16); i++)
      } else {
                     }
      ir_visitor_status
   lower_variables_visitor::visit(ir_variable *var)
   {
      if ((var->data.mode != ir_var_temporary &&
      var->data.mode != ir_var_auto &&
   /* Lower uniforms but not UBOs. */
   (var->data.mode != ir_var_uniform ||
      var->is_in_buffer_block() ||
   !(options->LowerPrecisionFloat16Uniforms &&
      !var->type->without_array()->is_32bit() ||
   (var->data.precision != GLSL_PRECISION_MEDIUM &&
   var->data.precision != GLSL_PRECISION_LOW) ||
   !can_lower_type(options, var->type))
         /* Lower constant initializers. */
   if (var->constant_value &&
      var->type == var->constant_value->type) {
   if (!options->LowerPrecisionConstants)
         var->constant_value =
                     if (var->constant_initializer &&
      var->type == var->constant_initializer->type) {
   if (!options->LowerPrecisionConstants)
         var->constant_initializer =
                     var->type = lower_glsl_type(var->type);
               }
      void
   lower_variables_visitor::fix_types_in_deref_chain(ir_dereference *ir)
   {
      assert(ir->type->without_array()->is_32bit());
            /* Fix the type in the dereference node. */
            /* If it's an array, fix the types in the whole dereference chain. */
   for (ir_dereference_array *deref_array = ir->as_dereference_array();
      deref_array;
   deref_array = deref_array->array->as_dereference_array()) {
   assert(deref_array->array->type->without_array()->is_32bit());
         }
      void
   lower_variables_visitor::convert_split_assignment(ir_dereference *lhs,
               {
               if (lhs->type->is_array()) {
      for (unsigned i = 0; i < lhs->type->length; i++) {
               l = new(mem_ctx) ir_dereference_array(lhs->clone(mem_ctx, NULL),
         r = new(mem_ctx) ir_dereference_array(rhs->clone(mem_ctx, NULL),
            }
               assert(lhs->type->is_16bit() || lhs->type->is_32bit());
   assert(rhs->type->is_16bit() || rhs->type->is_32bit());
            ir_assignment *assign =
            if (insert_before)
         else
      }
      ir_visitor_status
   lower_variables_visitor::visit_enter(ir_assignment *ir)
   {
      ir_dereference *lhs = ir->lhs;
   ir_variable *var = lhs->variable_referenced();
   ir_dereference *rhs_deref = ir->rhs->as_dereference();
   ir_variable *rhs_var = rhs_deref ? rhs_deref->variable_referenced() : NULL;
            /* Legalize array assignments between lowered and non-lowered variables. */
   if (lhs->type->is_array() &&
      (rhs_var || rhs_const) &&
   (!rhs_var ||
   (var &&
      var->type->without_array()->is_16bit() !=
      (!rhs_const ||
   (var &&
      var->type->without_array()->is_16bit() &&
               /* Fix array assignments from lowered to non-lowered. */
   if (rhs_var && _mesa_set_search(lower_vars, rhs_var)) {
      fix_types_in_deref_chain(rhs_deref);
   /* Convert to 32 bits for LHS. */
   convert_split_assignment(lhs, rhs_deref, true);
   ir->remove();
               /* Fix array assignments from non-lowered to lowered. */
   if (var &&
      _mesa_set_search(lower_vars, var) &&
   ir->rhs->type->without_array()->is_32bit()) {
   fix_types_in_deref_chain(lhs);
   /* Convert to 16 bits for LHS. */
   convert_split_assignment(lhs, ir->rhs, true);
   ir->remove();
                  /* Fix assignment types. */
   if (var &&
      _mesa_set_search(lower_vars, var)) {
   /* Fix the LHS type. */
   if (lhs->type->without_array()->is_32bit())
            /* Fix the RHS type if it's a lowered variable. */
   if (rhs_var &&
      _mesa_set_search(lower_vars, rhs_var) &&
               /* Fix the RHS type if it's a non-array expression. */
   if (ir->rhs->type->is_32bit()) {
               /* Convert the RHS to the LHS type. */
   if (expr &&
      (expr->operation == ir_unop_f162f ||
   expr->operation == ir_unop_i2i ||
   expr->operation == ir_unop_u2u) &&
   expr->operands[0]->type->is_16bit()) {
   /* If there is an "up" conversion, just remove it.
   * This is optional. We could as well execute the else statement and
   * let NIR eliminate the up+down conversions.
   */
      } else {
      /* Add a "down" conversion operation to fix the type of RHS. */
                        }
      ir_visitor_status
   lower_variables_visitor::visit_enter(ir_return *ir)
   {
               ir_dereference *deref = ir->value ? ir->value->as_dereference() : NULL;
   if (deref) {
               /* Fix the type of the return value. */
   if (var &&
      _mesa_set_search(lower_vars, var) &&
   deref->type->without_array()->is_32bit()) {
   /* Create a 32-bit temporary variable. */
   ir_variable *new_var =
                                 /* Convert to 32 bits for the return value. */
   convert_split_assignment(new(mem_ctx) ir_dereference_variable(new_var),
                           }
      void lower_variables_visitor::handle_rvalue(ir_rvalue **rvalue)
   {
               if (in_assignee || ir == NULL)
            ir_expression *expr = ir->as_expression();
            /* Remove f2fmp(float16). Same for int16 and uint16. */
   if (expr &&
      expr_op0_deref &&
   (expr->operation == ir_unop_f2fmp ||
   expr->operation == ir_unop_i2imp ||
   expr->operation == ir_unop_u2ump ||
   expr->operation == ir_unop_f2f16 ||
   expr->operation == ir_unop_i2i ||
   expr->operation == ir_unop_u2u) &&
   expr->type->without_array()->is_16bit() &&
   expr_op0_deref->type->without_array()->is_32bit() &&
   expr_op0_deref->variable_referenced() &&
   _mesa_set_search(lower_vars, expr_op0_deref->variable_referenced())) {
            /* Remove f2fmp/i2imp/u2ump. */
   *rvalue = expr_op0_deref;
                        if (deref) {
               /* var can be NULL if we are dereferencing ir_constant. */
   if (var &&
      _mesa_set_search(lower_vars, var) &&
                  /* Create a 32-bit temporary variable. */
   ir_variable *new_var =
                                 /* Convert to 32 bits for the rvalue. */
   convert_split_assignment(new(mem_ctx) ir_dereference_variable(new_var),
                  }
      ir_visitor_status
   lower_variables_visitor::visit_enter(ir_call *ir)
   {
               /* We can't pass 16-bit variables as 32-bit inout/out parameters. */
   foreach_two_lists(formal_node, &ir->callee->parameters,
            ir_dereference *param_deref =
                  if (!param_deref)
                     /* var can be NULL if we are dereferencing ir_constant. */
   if (var &&
      _mesa_set_search(lower_vars, var) &&
                  /* Create a 32-bit temporary variable for the parameter. */
   ir_variable *new_var =
                                 if (param->data.mode == ir_var_function_in ||
      param->data.mode == ir_var_function_inout) {
   /* Convert to 32 bits for passing in. */
   convert_split_assignment(new(mem_ctx) ir_dereference_variable(new_var),
      }
   if (param->data.mode == ir_var_function_out ||
      param->data.mode == ir_var_function_inout) {
   /* Convert to 16 bits after returning. */
   convert_split_assignment(param_deref,
                           /* Fix the type of return value dereferencies. */
   ir_dereference_variable *ret_deref = ir->return_deref;
            if (ret_var &&
      _mesa_set_search(lower_vars, ret_var) &&
   ret_deref->type->without_array()->is_32bit()) {
   /* Create a 32-bit temporary variable. */
   ir_variable *new_var =
      new(mem_ctx) ir_variable(ir->callee->return_type, "lowerp",
               /* Replace the return variable. */
            /* Convert to 16 bits after returning. */
   convert_split_assignment(new(mem_ctx) ir_dereference_variable(ret_var),
                        }
      }
      void
   lower_precision(const struct gl_shader_compiler_options *options,
         {
      find_precision_visitor v(options);
   find_lowerable_rvalues(options, instructions, v.lowerable_rvalues);
            lower_variables_visitor vars(options);
      }
