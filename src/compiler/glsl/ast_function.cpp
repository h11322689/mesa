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
   #include "ast.h"
   #include "compiler/glsl_types.h"
   #include "ir.h"
   #include "linker_util.h"
   #include "main/shader_types.h"
   #include "main/consts_exts.h"
   #include "main/shaderobj.h"
   #include "builtin_functions.h"
      static ir_rvalue *
   convert_component(ir_rvalue *src, const glsl_type *desired_type);
      static unsigned
   process_parameters(exec_list *instructions, exec_list *actual_parameters,
               {
      void *mem_ctx = state;
            foreach_list_typed(ast_node, ast, link, parameters) {
      /* We need to process the parameters first in order to know if we can
   * raise or not a unitialized warning. Calling set_is_lhs silence the
   * warning for now. Raising the warning or not will be checked at
   * verify_parameter_modes.
   */
   ast->set_is_lhs(true);
            /* Error happened processing function parameter */
   if (!result) {
      actual_parameters->push_tail(ir_rvalue::error_value(mem_ctx));
   count++;
               ir_constant *const constant =
            if (constant != NULL)
            actual_parameters->push_tail(result);
                  }
         /**
   * Generate a source prototype for a function signature
   *
   * \param return_type Return type of the function.  May be \c NULL.
   * \param name        Name of the function.
   * \param parameters  List of \c ir_instruction nodes representing the
   *                    parameter list for the function.  This may be either a
   *                    formal (\c ir_variable) or actual (\c ir_rvalue)
   *                    parameter list.  Only the type is used.
   *
   * \return
   * A ralloced string representing the prototype of the function.
   */
   char *
   prototype_string(const glsl_type *return_type, const char *name,
         {
               if (return_type != NULL)
                     const char *comma = "";
   foreach_in_list(const ir_variable, param, parameters) {
      ralloc_asprintf_append(&str, "%s%s", comma, glsl_get_type_name(param->type));
               ralloc_strcat(&str, ")");
      }
      static bool
   verify_image_parameter(YYLTYPE *loc, _mesa_glsl_parse_state *state,
         {
      /**
   * From the ARB_shader_image_load_store specification:
   *
   * "The values of image variables qualified with coherent,
   *  volatile, restrict, readonly, or writeonly may not be passed
   *  to functions whose formal parameters lack such
   *  qualifiers. [...] It is legal to have additional qualifiers
   *  on a formal parameter, but not to have fewer."
   */
   if (actual->data.memory_coherent && !formal->data.memory_coherent) {
      _mesa_glsl_error(loc, state,
                           if (actual->data.memory_volatile && !formal->data.memory_volatile) {
      _mesa_glsl_error(loc, state,
                           if (actual->data.memory_restrict && !formal->data.memory_restrict) {
      _mesa_glsl_error(loc, state,
                           if (actual->data.memory_read_only && !formal->data.memory_read_only) {
      _mesa_glsl_error(loc, state,
                           if (actual->data.memory_write_only && !formal->data.memory_write_only) {
      _mesa_glsl_error(loc, state,
                              }
      static bool
   verify_first_atomic_parameter(YYLTYPE *loc, _mesa_glsl_parse_state *state,
         {
      if (!var ||
      (!var->is_in_shader_storage_block() &&
   var->data.mode != ir_var_shader_shared)) {
   _mesa_glsl_error(loc, state, "First argument to atomic function "
            }
      }
      static bool
   is_atomic_function(const char *func_name)
   {
      return !strcmp(func_name, "atomicAdd") ||
         !strcmp(func_name, "atomicMin") ||
   !strcmp(func_name, "atomicMax") ||
   !strcmp(func_name, "atomicAnd") ||
   !strcmp(func_name, "atomicOr") ||
   !strcmp(func_name, "atomicXor") ||
      }
      static bool
   verify_atomic_image_parameter_qualifier(YYLTYPE *loc, _mesa_glsl_parse_state *state,
         {
      if (!var ||
      (var->data.image_format != PIPE_FORMAT_R32_UINT &&
   var->data.image_format != PIPE_FORMAT_R32_SINT &&
   var->data.image_format != PIPE_FORMAT_R32_FLOAT)) {
   _mesa_glsl_error(loc, state, "Image atomic functions should use r32i/r32ui "
            }
      }
      static bool
   is_atomic_image_function(const char *func_name)
   {
      return !strcmp(func_name, "imageAtomicAdd") ||
         !strcmp(func_name, "imageAtomicMin") ||
   !strcmp(func_name, "imageAtomicMax") ||
   !strcmp(func_name, "imageAtomicAnd") ||
   !strcmp(func_name, "imageAtomicOr") ||
   !strcmp(func_name, "imageAtomicXor") ||
   !strcmp(func_name, "imageAtomicExchange") ||
   !strcmp(func_name, "imageAtomicCompSwap") ||
      }
         /**
   * Verify that 'out' and 'inout' actual parameters are lvalues.  Also, verify
   * that 'const_in' formal parameters (an extension in our IR) correspond to
   * ir_constant actual parameters.
   */
   static bool
   verify_parameter_modes(_mesa_glsl_parse_state *state,
                     {
      exec_node *actual_ir_node  = actual_ir_parameters.get_head_raw();
            foreach_in_list(const ir_variable, formal, &sig->parameters) {
      /* The lists must be the same length. */
   assert(!actual_ir_node->is_tail_sentinel());
            const ir_rvalue *const actual = (ir_rvalue *) actual_ir_node;
   const ast_expression *const actual_ast =
                     /* Verify that 'const_in' parameters are ir_constants. */
   if (formal->data.mode == ir_var_const_in &&
      actual->ir_type != ir_type_constant) {
   _mesa_glsl_error(&loc, state,
                           /* Verify that shader_in parameters are shader inputs */
   if (formal->data.must_be_shader_input) {
               /* GLSL 4.40 allows swizzles, while earlier GLSL versions do not. */
   if (val->ir_type == ir_type_swizzle) {
      if (!state->is_version(440, 0)) {
      _mesa_glsl_error(&loc, state,
                  }
               for (;;) {
      if (val->ir_type == ir_type_dereference_array) {
         } else if (val->ir_type == ir_type_dereference_record &&
               } else
               ir_variable *var = NULL;
                  if (!var || var->data.mode != ir_var_shader_in) {
      _mesa_glsl_error(&loc, state,
                                       /* Verify that 'out' and 'inout' actual parameters are lvalues. */
   if (formal->data.mode == ir_var_function_out
      || formal->data.mode == ir_var_function_inout) {
   const char *mode = NULL;
   switch (formal->data.mode) {
   case ir_var_function_out:   mode = "out";   break;
   case ir_var_function_inout: mode = "inout"; break;
                  /* This AST-based check catches errors like f(i++).  The IR-based
   * is_lvalue() is insufficient because the actual parameter at the
   * IR-level is just a temporary value, which is an l-value.
   */
   if (actual_ast->non_lvalue_description != NULL) {
      _mesa_glsl_error(&loc, state,
                                          if (var && formal->data.mode == ir_var_function_inout) {
      if ((var->data.mode == ir_var_auto ||
      var->data.mode == ir_var_shader_out) &&
   !var->data.assigned &&
   !is_gl_identifier(var->name)) {
   _mesa_glsl_warning(&loc, state, "`%s' used uninitialized",
                                 if (var && var->data.read_only) {
      _mesa_glsl_error(&loc, state,
                  "function parameter '%s %s' references the "
         } else if (!actual->is_lvalue(state)) {
      _mesa_glsl_error(&loc, state,
                     } else {
      assert(formal->data.mode == ir_var_function_in ||
         ir_variable *var = actual->variable_referenced();
   if (var) {
      if ((var->data.mode == ir_var_auto ||
      var->data.mode == ir_var_shader_out) &&
   !var->data.assigned &&
   !is_gl_identifier(var->name)) {
   _mesa_glsl_warning(&loc, state, "`%s' used uninitialized",
                     if (formal->type->is_image() &&
      actual->variable_referenced()) {
   if (!verify_image_parameter(&loc, state, formal,
                     actual_ir_node  = actual_ir_node->next;
               /* The first parameter of atomic functions must be a buffer variable */
   const char *func_name = sig->function_name();
   bool is_atomic = is_atomic_function(func_name);
   if (is_atomic) {
      const ir_rvalue *const actual =
            const ast_expression *const actual_ast =
      exec_node_data(ast_expression,
               if (!verify_first_atomic_parameter(&loc, state,
                  } else if (is_atomic_image_function(func_name)) {
      const ir_rvalue *const actual =
            const ast_expression *const actual_ast =
      exec_node_data(ast_expression,
               if (!verify_atomic_image_parameter_qualifier(&loc, state,
                              }
      struct copy_index_deref_data {
      void *mem_ctx;
      };
      static void
   copy_index_derefs_to_temps(ir_instruction *ir, void *data)
   {
               if (ir->ir_type == ir_type_dereference_array) {
      ir_dereference_array *a = (ir_dereference_array *) ir;
            ir_rvalue *idx = a->array_index;
            /* If the index is read only it cannot change so there is no need
   * to copy it.
   */
   if (!var || var->data.read_only || var->data.memory_read_only)
            ir_variable *tmp = new(d->mem_ctx) ir_variable(idx->type, "idx_tmp",
                  ir_dereference_variable *const deref_tmp_1 =
         ir_assignment *const assignment =
      new(d->mem_ctx) ir_assignment(deref_tmp_1,
               /* Replace the array index with a dereference of the new temporary */
   ir_dereference_variable *const deref_tmp_2 =
               }
      static void
   fix_parameter(void *mem_ctx, ir_rvalue *actual, const glsl_type *formal_type,
               {
               /* If the types match exactly and the parameter is not a vector-extract,
   * nothing needs to be done to fix the parameter.
   */
   if (formal_type == actual->type
      && (expr == NULL || expr->operation != ir_binop_vector_extract)
   && actual->as_dereference_variable())
         /* An array index could also be an out variable so we need to make a copy
   * of them before the function is called.
   */
   if (!actual->as_dereference_variable()) {
      struct copy_index_deref_data data;
   data.mem_ctx = mem_ctx;
                        /* To convert an out parameter, we need to create a temporary variable to
   * hold the value before conversion, and then perform the conversion after
   * the function call returns.
   *
   * This has the effect of transforming code like this:
   *
   *   void f(out int x);
   *   float value;
   *   f(value);
   *
   * Into IR that's equivalent to this:
   *
   *   void f(out int x);
   *   float value;
   *   int out_parameter_conversion;
   *   f(out_parameter_conversion);
   *   value = float(out_parameter_conversion);
   *
   * If the parameter is an ir_expression of ir_binop_vector_extract,
   * additional conversion is needed in the post-call re-write.
   */
   ir_variable *tmp =
                     /* If the parameter is an inout parameter, copy the value of the actual
   * parameter to the new temporary.  Note that no type conversion is allowed
   * here because inout parameters must match types exactly.
   */
   if (parameter_is_inout) {
      /* Inout parameters should never require conversion, since that would
   * require an implicit conversion to exist both to and from the formal
   * parameter type, and there are no bidirectional implicit conversions.
   */
            ir_dereference_variable *const deref_tmp_1 =
         ir_assignment *const assignment =
                     /* Replace the parameter in the call with a dereference of the new
   * temporary.
   */
   ir_dereference_variable *const deref_tmp_2 =
                     /* Copy the temporary variable to the actual parameter with optional
   * type conversion applied.
   */
   ir_rvalue *rhs = new(mem_ctx) ir_dereference_variable(tmp);
   if (actual->type != formal_type)
            ir_rvalue *lhs = actual;
   if (expr != NULL && expr->operation == ir_binop_vector_extract) {
      lhs = new(mem_ctx) ir_dereference_array(expr->operands[0]->clone(mem_ctx,
                           ir_assignment *const assignment_2 = new(mem_ctx) ir_assignment(lhs, rhs);
      }
      /**
   * Generate a function call.
   *
   * For non-void functions, this returns a dereference of the temporary
   * variable which stores the return value for the call.  For void functions,
   * this returns NULL.
   */
   static ir_rvalue *
   generate_call(exec_list *instructions, ir_function_signature *sig,
               exec_list *actual_parameters,
   ir_variable *sub_var,
   {
      void *ctx = state;
            /* Perform implicit conversion of arguments.  For out parameters, we need
   * to place them in a temporary variable and do the conversion after the
   * call takes place.  Since we haven't emitted the call yet, we'll place
   * the post-call conversions in a temporary exec_list, and emit them later.
   */
   foreach_two_lists(formal_node, &sig->parameters,
            ir_rvalue *actual = (ir_rvalue *) actual_node;
            if (formal->type->is_numeric() || formal->type->is_boolean()) {
      switch (formal->data.mode) {
   case ir_var_const_in:
   case ir_var_function_in: {
      ir_rvalue *converted
         actual->replace_with(converted);
      }
   case ir_var_function_out:
   case ir_var_function_inout:
      fix_parameter(ctx, actual, formal->type,
                  default:
      assert (!"Illegal formal parameter mode");
                     /* Section 4.3.2 (Const) of the GLSL 1.10.59 spec says:
   *
   *     "Initializers for const declarations must be formed from literal
   *     values, other const variables (not including function call
   *     paramaters), or expressions of these.
   *
   *     Constructors may be used in such expressions, but function calls may
   *     not."
   *
   * Section 4.3.3 (Constant Expressions) of the GLSL 1.20.8 spec says:
   *
   *     "A constant expression is one of
   *
   *         ...
   *
   *         - a built-in function call whose arguments are all constant
   *           expressions, with the exception of the texture lookup
   *           functions, the noise functions, and ftransform. The built-in
   *           functions dFdx, dFdy, and fwidth must return 0 when evaluated
   *           inside an initializer with an argument that is a constant
   *           expression."
   *
   * Section 5.10 (Constant Expressions) of the GLSL ES 1.00.17 spec says:
   *
   *     "A constant expression is one of
   *
   *         ...
   *
   *         - a built-in function call whose arguments are all constant
   *           expressions, with the exception of the texture lookup
   *           functions."
   *
   * Section 4.3.3 (Constant Expressions) of the GLSL ES 3.00.4 spec says:
   *
   *     "A constant expression is one of
   *
   *         ...
   *
   *         - a built-in function call whose arguments are all constant
   *           expressions, with the exception of the texture lookup
   *           functions.  The built-in functions dFdx, dFdy, and fwidth must
   *           return 0 when evaluated inside an initializer with an argument
   *           that is a constant expression."
   *
   * If the function call is a constant expression, don't generate any
   * instructions; just generate an ir_constant.
   */
   if (state->is_version(120, 100) ||
      state->consts->AllowGLSLBuiltinConstantExpression) {
   ir_constant *value = sig->constant_expression_value(ctx,
               if (value != NULL) {
                     ir_dereference_variable *deref = NULL;
   if (!sig->return_type->is_void()) {
      /* Create a new temporary to hold the return value. */
   char *const name = ir_variable::temporaries_allocate_names
                           var = new(ctx) ir_variable(sig->return_type, name, ir_var_temporary);
   var->data.precision = sig->return_precision;
                                 ir_call *call = new(ctx) ir_call(sig, deref,
                  /* Also emit any necessary out-parameter conversions. */
               }
      /**
   * Given a function name and parameter list, find the matching signature.
   */
   static ir_function_signature *
   match_function_by_name(const char *name,
               {
      ir_function *f = state->symbols->get_function(name);
   ir_function_signature *local_sig = NULL;
            /* Is the function hidden by a record type constructor? */
   if (state->symbols->get_type(name))
            /* Is the function hidden by a variable (impossible in 1.10)? */
   if (!state->symbols->separate_function_namespace
      && state->symbols->get_variable(name))
         if (f != NULL) {
      /* In desktop GL, the presence of a user-defined signature hides any
   * built-in signatures, so we must ignore them.  In contrast, in ES2
   * user-defined signatures add new overloads, so we must consider them.
   */
            /* Look for a match in the local shader.  If exact, we're done. */
   bool is_exact = false;
   sig = local_sig = f->matching_signature(state, actual_parameters,
         if (is_exact)
            if (!allow_builtins)
               /* Local shader has no exact candidates; check the built-ins. */
            /* if _mesa_glsl_find_builtin_function failed, fall back to the result
   * of choose_best_inexact_overload() instead. This should only affect
   * GLES.
   */
      }
      static ir_function_signature *
   match_subroutine_by_name(const char *name,
                     {
      void *ctx = state;
   ir_function_signature *sig = NULL;
   ir_function *f, *found = NULL;
   const char *new_name;
   ir_variable *var;
            new_name =
      ralloc_asprintf(ctx, "%s_%s",
            var = state->symbols->get_variable(new_name);
   if (!var)
            for (int i = 0; i < state->num_subroutine_types; i++) {
      f = state->subroutine_types[i];
   if (strcmp(f->name, glsl_get_type_name(var->type->without_array())))
         found = f;
               if (!found)
         *var_r = var;
   sig = found->matching_signature(state, actual_parameters,
            }
      static ir_rvalue *
   generate_array_index(void *mem_ctx, exec_list *instructions,
                     {
      if (array->oper == ast_array_index) {
      /* This handles arrays of arrays */
   ir_rvalue *outer_array = generate_array_index(mem_ctx, instructions,
                                          YYLTYPE index_loc = idx->get_location();
   return _mesa_ast_array_index_to_hir(mem_ctx, state, outer_array,
            } else {
      ir_variable *sub_var = NULL;
            if (!match_subroutine_by_name(*function_name, actual_parameters,
            _mesa_glsl_error(&loc, state, "Unknown subroutine `%s'",
         *function_name = NULL; /* indicate error condition to caller */
               ir_rvalue *outer_array_idx = idx->hir(instructions, state);
         }
      static bool
   function_exists(_mesa_glsl_parse_state *state,
         {
      ir_function *f = symbols->get_function(name);
   if (f != NULL) {
      foreach_in_list(ir_function_signature, sig, &f->signatures) {
      if (sig->is_builtin() && !sig->is_builtin_available(state))
               }
      }
      static void
   print_function_prototypes(_mesa_glsl_parse_state *state, YYLTYPE *loc,
         {
      if (f == NULL)
            foreach_in_list(ir_function_signature, sig, &f->signatures) {
      if (sig->is_builtin() && !sig->is_builtin_available(state))
            char *str = prototype_string(sig->return_type, f->name,
         _mesa_glsl_error(loc, state, "   %s", str);
         }
      /**
   * Raise a "no matching function" error, listing all possible overloads the
   * compiler considered so developers can figure out what went wrong.
   */
   static void
   no_matching_function_error(const char *name,
                     {
               if (!function_exists(state, state->symbols, name)
      && (!state->uses_builtin_functions
            } else {
      char *str = prototype_string(NULL, name, actual_parameters);
   _mesa_glsl_error(loc, state,
                              print_function_prototypes(state, loc,
            if (state->uses_builtin_functions) {
      print_function_prototypes(state, loc,
            }
      /**
   * Perform automatic type conversion of constructor parameters
   *
   * This implements the rules in the "Conversion and Scalar Constructors"
   * section (GLSL 1.10 section 5.4.1), not the "Implicit Conversions" rules.
   */
   static ir_rvalue *
   convert_component(ir_rvalue *src, const glsl_type *desired_type)
   {
      void *ctx = ralloc_parent(src);
   const unsigned a = desired_type->base_type;
   const unsigned b = src->type->base_type;
            if (src->type->is_error())
            assert(a <= GLSL_TYPE_IMAGE);
            if (a == b)
            switch (a) {
   case GLSL_TYPE_UINT:
      switch (b) {
   case GLSL_TYPE_INT:
      result = new(ctx) ir_expression(ir_unop_i2u, src);
      case GLSL_TYPE_FLOAT:
      result = new(ctx) ir_expression(ir_unop_f2u, src);
      case GLSL_TYPE_BOOL:
      result = new(ctx) ir_expression(ir_unop_i2u,
                  case GLSL_TYPE_DOUBLE:
      result = new(ctx) ir_expression(ir_unop_d2u, src);
      case GLSL_TYPE_UINT64:
      result = new(ctx) ir_expression(ir_unop_u642u, src);
      case GLSL_TYPE_INT64:
      result = new(ctx) ir_expression(ir_unop_i642u, src);
      case GLSL_TYPE_SAMPLER:
      result = new(ctx) ir_expression(ir_unop_unpack_sampler_2x32, src);
      case GLSL_TYPE_IMAGE:
      result = new(ctx) ir_expression(ir_unop_unpack_image_2x32, src);
      }
      case GLSL_TYPE_INT:
      switch (b) {
   case GLSL_TYPE_UINT:
      result = new(ctx) ir_expression(ir_unop_u2i, src);
      case GLSL_TYPE_FLOAT:
      result = new(ctx) ir_expression(ir_unop_f2i, src);
      case GLSL_TYPE_BOOL:
      result = new(ctx) ir_expression(ir_unop_b2i, src);
      case GLSL_TYPE_DOUBLE:
      result = new(ctx) ir_expression(ir_unop_d2i, src);
      case GLSL_TYPE_UINT64:
      result = new(ctx) ir_expression(ir_unop_u642i, src);
      case GLSL_TYPE_INT64:
      result = new(ctx) ir_expression(ir_unop_i642i, src);
      }
      case GLSL_TYPE_FLOAT:
      switch (b) {
   case GLSL_TYPE_UINT:
      result = new(ctx) ir_expression(ir_unop_u2f, desired_type, src, NULL);
      case GLSL_TYPE_INT:
      result = new(ctx) ir_expression(ir_unop_i2f, desired_type, src, NULL);
      case GLSL_TYPE_BOOL:
      result = new(ctx) ir_expression(ir_unop_b2f, desired_type, src, NULL);
      case GLSL_TYPE_DOUBLE:
      result = new(ctx) ir_expression(ir_unop_d2f, desired_type, src, NULL);
      case GLSL_TYPE_UINT64:
      result = new(ctx) ir_expression(ir_unop_u642f, desired_type, src, NULL);
      case GLSL_TYPE_INT64:
      result = new(ctx) ir_expression(ir_unop_i642f, desired_type, src, NULL);
      }
      case GLSL_TYPE_BOOL:
      switch (b) {
   case GLSL_TYPE_UINT:
      result = new(ctx) ir_expression(ir_unop_i2b,
                  case GLSL_TYPE_INT:
      result = new(ctx) ir_expression(ir_unop_i2b, desired_type, src, NULL);
      case GLSL_TYPE_FLOAT:
      result = new(ctx) ir_expression(ir_unop_f2b, desired_type, src, NULL);
      case GLSL_TYPE_DOUBLE:
      result = new(ctx) ir_expression(ir_unop_d2b, desired_type, src, NULL);
      case GLSL_TYPE_UINT64:
      result = new(ctx) ir_expression(ir_unop_i642b,
                  case GLSL_TYPE_INT64:
      result = new(ctx) ir_expression(ir_unop_i642b, desired_type, src, NULL);
      }
      case GLSL_TYPE_DOUBLE:
      switch (b) {
   case GLSL_TYPE_INT:
      result = new(ctx) ir_expression(ir_unop_i2d, src);
      case GLSL_TYPE_UINT:
      result = new(ctx) ir_expression(ir_unop_u2d, src);
      case GLSL_TYPE_BOOL:
      result = new(ctx) ir_expression(ir_unop_f2d,
                  case GLSL_TYPE_FLOAT:
      result = new(ctx) ir_expression(ir_unop_f2d, desired_type, src, NULL);
      case GLSL_TYPE_UINT64:
      result = new(ctx) ir_expression(ir_unop_u642d, desired_type, src, NULL);
      case GLSL_TYPE_INT64:
      result = new(ctx) ir_expression(ir_unop_i642d, desired_type, src, NULL);
      }
      case GLSL_TYPE_UINT64:
      switch (b) {
   case GLSL_TYPE_INT:
      result = new(ctx) ir_expression(ir_unop_i2u64, src);
      case GLSL_TYPE_UINT:
      result = new(ctx) ir_expression(ir_unop_u2u64, src);
      case GLSL_TYPE_BOOL:
      result = new(ctx) ir_expression(ir_unop_i642u64,
                  case GLSL_TYPE_FLOAT:
      result = new(ctx) ir_expression(ir_unop_f2u64, src);
      case GLSL_TYPE_DOUBLE:
      result = new(ctx) ir_expression(ir_unop_d2u64, src);
      case GLSL_TYPE_INT64:
      result = new(ctx) ir_expression(ir_unop_i642u64, src);
      }
      case GLSL_TYPE_INT64:
      switch (b) {
   case GLSL_TYPE_INT:
      result = new(ctx) ir_expression(ir_unop_i2i64, src);
      case GLSL_TYPE_UINT:
      result = new(ctx) ir_expression(ir_unop_u2i64, src);
      case GLSL_TYPE_BOOL:
      result = new(ctx) ir_expression(ir_unop_b2i64, src);
      case GLSL_TYPE_FLOAT:
      result = new(ctx) ir_expression(ir_unop_f2i64, src);
      case GLSL_TYPE_DOUBLE:
      result = new(ctx) ir_expression(ir_unop_d2i64, src);
      case GLSL_TYPE_UINT64:
      result = new(ctx) ir_expression(ir_unop_u642i64, src);
      }
      case GLSL_TYPE_SAMPLER:
      switch (b) {
   case GLSL_TYPE_UINT:
      result = new(ctx)
            }
      case GLSL_TYPE_IMAGE:
      switch (b) {
   case GLSL_TYPE_UINT:
      result = new(ctx)
            }
               assert(result != NULL);
            /* Try constant folding; it may fold in the conversion we just added. */
   ir_constant *const constant = result->constant_expression_value(ctx);
      }
         /**
   * Perform automatic type and constant conversion of constructor parameters
   *
   * This implements the rules in the "Implicit Conversions" rules, not the
   * "Conversion and Scalar Constructors".
   *
   * After attempting the implicit conversion, an attempt to convert into a
   * constant valued expression is also done.
   *
   * The \c from \c ir_rvalue is converted "in place".
   *
   * \param from   Operand that is being converted
   * \param to     Base type the operand will be converted to
   * \param state  GLSL compiler state
   *
   * \return
   * If the attempt to convert into a constant expression succeeds, \c true is
   * returned. Otherwise \c false is returned.
   */
   static bool
   implicitly_convert_component(ir_rvalue * &from, const glsl_base_type to,
         {
      void *mem_ctx = state;
            if (to != from->type->base_type) {
      const glsl_type *desired_type =
      glsl_type::get_instance(to,
               if (_mesa_glsl_can_implicitly_convert(from->type, desired_type, state)) {
      /* Even though convert_component() implements the constructor
   * conversion rules (not the implicit conversion rules), its safe
   * to use it here because we already checked that the implicit
   * conversion is legal.
   */
                           if (constant != NULL)
            if (from != result) {
      from->replace_with(result);
                  }
         /**
   * Dereference a specific component from a scalar, vector, or matrix
   */
   static ir_rvalue *
   dereference_component(ir_rvalue *src, unsigned component)
   {
      void *ctx = ralloc_parent(src);
            /* If the source is a constant, just create a new constant instead of a
   * dereference of the existing constant.
   */
   ir_constant *constant = src->as_constant();
   if (constant)
            if (src->type->is_scalar()) {
         } else if (src->type->is_vector()) {
         } else {
               /* Dereference a row of the matrix, then call this function again to get
   * a specific element from that row.
   */
   const int c = component / src->type->column_type()->vector_elements;
   const int r = component % src->type->column_type()->vector_elements;
   ir_constant *const col_index = new(ctx) ir_constant(c);
   ir_dereference *const col = new(ctx) ir_dereference_array(src,
                                 assert(!"Should not get here.");
      }
         static ir_rvalue *
   process_vec_mat_constructor(exec_list *instructions,
                     {
               /* The ARB_shading_language_420pack spec says:
   *
   * "If an initializer is a list of initializers enclosed in curly braces,
   *  the variable being declared must be a vector, a matrix, an array, or a
   *  structure.
   *
   *      int i = { 1 }; // illegal, i is not an aggregate"
   */
   if (constructor_type->vector_elements <= 1) {
      _mesa_glsl_error(loc, state, "aggregates can only initialize vectors, "
                     exec_list actual_parameters;
   const unsigned parameter_count =
            if (parameter_count == 0
      || (constructor_type->is_vector() &&
         || (constructor_type->is_matrix() &&
         _mesa_glsl_error(loc, state, "%s constructor must have %u parameters",
                                    /* Type cast each parameter and, if possible, fold constants. */
   foreach_in_list_safe(ir_rvalue, ir, &actual_parameters) {
      /* Apply implicit conversions (not the scalar constructor rules, see the
   * spec quote above!) and attempt to convert the parameter to a constant
   * valued expression. After doing so, track whether or not all the
   * parameters to the constructor are trivially constant valued
   * expressions.
   */
   all_parameters_are_constant &=
            if (constructor_type->is_matrix()) {
      if (ir->type != constructor_type->column_type()) {
      _mesa_glsl_error(loc, state, "type error in matrix constructor: "
                           } else if (ir->type != constructor_type->get_scalar_type()) {
      _mesa_glsl_error(loc, state, "type error in vector constructor: "
                                    if (all_parameters_are_constant)
            ir_variable *var = new(ctx) ir_variable(constructor_type, "vec_mat_ctor",
                           foreach_in_list(ir_rvalue, rhs, &actual_parameters) {
               if (var->type->is_matrix()) {
      ir_rvalue *lhs =
            } else {
      /* use writemask rather than index for vector */
   assert(var->type->is_vector());
   assert(i < 4);
   ir_dereference *lhs = new(ctx) ir_dereference_variable(var);
                                       }
         static ir_rvalue *
   process_array_constructor(exec_list *instructions,
                     {
      void *ctx = state;
   /* Array constructors come in two forms: sized and unsized.  Sized array
   * constructors look like 'vec4[2](a, b)', where 'a' and 'b' are vec4
   * variables.  In this case the number of parameters must exactly match the
   * specified size of the array.
   *
   * Unsized array constructors look like 'vec4[](a, b)', where 'a' and 'b'
   * are vec4 variables.  In this case the size of the array being constructed
   * is determined by the number of parameters.
   *
   * From page 52 (page 58 of the PDF) of the GLSL 1.50 spec:
   *
   *    "There must be exactly the same number of arguments as the size of
   *    the array being constructed. If no size is present in the
   *    constructor, then the array is explicitly sized to the number of
   *    arguments provided. The arguments are assigned in order, starting at
   *    element 0, to the elements of the constructed array. Each argument
   *    must be the same type as the element type of the array, or be a type
   *    that can be converted to the element type of the array according to
   *    Section 4.1.10 "Implicit Conversions.""
   */
   exec_list actual_parameters;
   const unsigned parameter_count =
                  if ((parameter_count == 0) ||
      (!is_unsized_array && (constructor_type->length != parameter_count))) {
   const unsigned min_param = is_unsized_array
            _mesa_glsl_error(loc, state, "array constructor must have %s %u "
                                 if (is_unsized_array) {
      constructor_type =
      glsl_type::get_array_instance(constructor_type->fields.array,
      assert(constructor_type != NULL);
               bool all_parameters_are_constant = true;
            /* Type cast each parameter and, if possible, fold constants. */
   foreach_in_list_safe(ir_rvalue, ir, &actual_parameters) {
      /* Apply implicit conversions (not the scalar constructor rules, see the
   * spec quote above!) and attempt to convert the parameter to a constant
   * valued expression. After doing so, track whether or not all the
   * parameters to the constructor are trivially constant valued
   * expressions.
   */
   all_parameters_are_constant &=
            if (constructor_type->fields.array->is_unsized_array()) {
      /* As the inner parameters of the constructor are created without
   * knowledge of each other we need to check to make sure unsized
   * parameters of unsized constructors all end up with the same size.
   *
   * e.g we make sure to fail for a constructor like this:
   * vec4[][] a = vec4[][](vec4[](vec4(0.0), vec4(1.0)),
   *                       vec4[](vec4(0.0), vec4(1.0), vec4(1.0)),
   *                       vec4[](vec4(0.0), vec4(1.0)));
   */
   if (element_type->is_unsized_array()) {
      /* This is the first parameter so just get the type */
      } else if (element_type != ir->type) {
      _mesa_glsl_error(loc, state, "type error in array constructor: "
                           } else if (ir->type != constructor_type->fields.array) {
      _mesa_glsl_error(loc, state, "type error in array constructor: "
                        } else {
                     if (constructor_type->fields.array->is_unsized_array()) {
      constructor_type =
      glsl_type::get_array_instance(element_type,
      assert(constructor_type != NULL);
               if (all_parameters_are_constant)
            ir_variable *var = new(ctx) ir_variable(constructor_type, "array_ctor",
                  int i = 0;
   foreach_in_list(ir_rvalue, rhs, &actual_parameters) {
      ir_rvalue *lhs = new(ctx) ir_dereference_array(var,
            ir_instruction *assignment = new(ctx) ir_assignment(lhs, rhs);
                           }
         /**
   * Determine if a list consists of a single scalar r-value
   */
   static bool
   single_scalar_parameter(exec_list *parameters)
   {
      const ir_rvalue *const p = (ir_rvalue *) parameters->get_head_raw();
               }
         /**
   * Generate inline code for a vector constructor
   *
   * The generated constructor code will consist of a temporary variable
   * declaration of the same type as the constructor.  A sequence of assignments
   * from constructor parameters to the temporary will follow.
   *
   * \return
   * An \c ir_dereference_variable of the temprorary generated in the constructor
   * body.
   */
   static ir_rvalue *
   emit_inline_vector_constructor(const glsl_type *type,
                     {
               ir_variable *var = new(ctx) ir_variable(type, "vec_ctor", ir_var_temporary);
            /* There are three kinds of vector constructors.
   *
   *  - Construct a vector from a single scalar by replicating that scalar to
   *    all components of the vector.
   *
   *  - Construct a vector from at least a matrix. This case should already
   *    have been taken care of in ast_function_expression::hir by breaking
   *    down the matrix into a series of column vectors.
   *
   *  - Construct a vector from an arbirary combination of vectors and
   *    scalars.  The components of the constructor parameters are assigned
   *    to the vector in order until the vector is full.
   */
   const unsigned lhs_components = type->components();
   if (single_scalar_parameter(parameters)) {
      ir_rvalue *first_param = (ir_rvalue *)parameters->get_head_raw();
      } else {
      unsigned base_component = 0;
   unsigned base_lhs_component = 0;
   ir_constant_data data;
                     foreach_in_list(ir_rvalue, param, parameters) {
               /* Do not try to assign more components to the vector than it has! */
   if ((rhs_components + base_lhs_component) > lhs_components) {
                  const ir_constant *const c = param->as_constant();
   if (c != NULL) {
      for (unsigned i = 0; i < rhs_components; i++) {
      switch (c->type->base_type) {
   case GLSL_TYPE_UINT:
      data.u[i + base_component] = c->get_uint_component(i);
      case GLSL_TYPE_INT:
      data.i[i + base_component] = c->get_int_component(i);
      case GLSL_TYPE_FLOAT:
      data.f[i + base_component] = c->get_float_component(i);
      case GLSL_TYPE_DOUBLE:
      data.d[i + base_component] = c->get_double_component(i);
      case GLSL_TYPE_BOOL:
      data.b[i + base_component] = c->get_bool_component(i);
      case GLSL_TYPE_UINT64:
      data.u64[i + base_component] = c->get_uint64_component(i);
      case GLSL_TYPE_INT64:
      data.i64[i + base_component] = c->get_int64_component(i);
      default:
      assert(!"Should not get here.");
                  /* Mask of fields to be written in the assignment. */
                     }
   /* Advance the component index by the number of components
   * that were just assigned.
   */
               if (constant_mask != 0) {
      ir_dereference *lhs = new(ctx) ir_dereference_variable(var);
   const glsl_type *rhs_type =
      glsl_type::get_instance(var->type->base_type,
                     ir_instruction *inst =
                     base_component = 0;
   foreach_in_list(ir_rvalue, param, parameters) {
               /* Do not try to assign more components to the vector than it has! */
   if ((rhs_components + base_component) > lhs_components) {
                  /* If we do not have any components left to copy, break out of the
   * loop. This can happen when initializing a vec4 with a mat3 as the
   * mat3 would have been broken into a series of column vectors.
   */
   if (rhs_components == 0) {
                  const ir_constant *const c = param->as_constant();
   if (c == NULL) {
      /* Mask of fields to be written in the assignment. */
                           /* Generate a swizzle so that LHS and RHS sizes match. */
                  ir_instruction *inst =
                     /* Advance the component index by the number of components that were
   * just assigned.
   */
         }
      }
         /**
   * Generate assignment of a portion of a vector to a portion of a matrix column
   *
   * \param src_base  First component of the source to be used in assignment
   * \param column    Column of destination to be assiged
   * \param row_base  First component of the destination column to be assigned
   * \param count     Number of components to be assigned
   *
   * \note
   * \c src_base + \c count must be less than or equal to the number of
   * components in the source vector.
   */
   static ir_instruction *
   assign_to_matrix_column(ir_variable *var, unsigned column, unsigned row_base,
               {
      ir_constant *col_idx = new(mem_ctx) ir_constant(column);
   ir_dereference *column_ref = new(mem_ctx) ir_dereference_array(var,
            assert(column_ref->type->components() >= (row_base + count));
            /* Generate a swizzle that extracts the number of components from the source
   * that are to be assigned to the column of the matrix.
   */
   if (count < src->type->vector_elements) {
      src = new(mem_ctx) ir_swizzle(src,
                           /* Mask of fields to be written in the assignment. */
               }
         /**
   * Generate inline code for a matrix constructor
   *
   * The generated constructor code will consist of a temporary variable
   * declaration of the same type as the constructor.  A sequence of assignments
   * from constructor parameters to the temporary will follow.
   *
   * \return
   * An \c ir_dereference_variable of the temprorary generated in the constructor
   * body.
   */
   static ir_rvalue *
   emit_inline_matrix_constructor(const glsl_type *type,
                     {
               ir_variable *var = new(ctx) ir_variable(type, "mat_ctor", ir_var_temporary);
            /* There are three kinds of matrix constructors.
   *
   *  - Construct a matrix from a single scalar by replicating that scalar to
   *    along the diagonal of the matrix and setting all other components to
   *    zero.
   *
   *  - Construct a matrix from an arbirary combination of vectors and
   *    scalars.  The components of the constructor parameters are assigned
   *    to the matrix in column-major order until the matrix is full.
   *
   *  - Construct a matrix from a single matrix.  The source matrix is copied
   *    to the upper left portion of the constructed matrix, and the remaining
   *    elements take values from the identity matrix.
   */
   ir_rvalue *const first_param = (ir_rvalue *) parameters->get_head_raw();
   if (single_scalar_parameter(parameters)) {
      /* Assign the scalar to the X component of a vec4, and fill the remaining
   * components with zero.
   */
   glsl_base_type param_base_type = first_param->type->base_type;
   assert(first_param->type->is_float() || first_param->type->is_double());
   ir_variable *rhs_var =
      new(ctx) ir_variable(glsl_type::get_instance(param_base_type, 4, 1),
                     ir_constant_data zero;
   for (unsigned i = 0; i < 4; i++)
      if (first_param->type->is_float())
                     ir_instruction *inst =
      new(ctx) ir_assignment(new(ctx) ir_dereference_variable(rhs_var),
               ir_dereference *const rhs_ref =
            inst = new(ctx) ir_assignment(rhs_ref, first_param, 0x01);
            /* Assign the temporary vector to each column of the destination matrix
   * with a swizzle that puts the X component on the diagonal of the
   * matrix.  In some cases this may mean that the X component does not
   * get assigned into the column at all (i.e., when the matrix has more
   * columns than rows).
   */
   static const unsigned rhs_swiz[4][4] = {
      { 0, 1, 1, 1 },
   { 1, 0, 1, 1 },
   { 1, 1, 0, 1 },
               const unsigned cols_to_init = MIN2(type->matrix_columns,
         for (unsigned i = 0; i < cols_to_init; i++) {
      ir_constant *const col_idx = new(ctx) ir_constant(i);
                  ir_rvalue *const rhs_ref = new(ctx) ir_dereference_variable(rhs_var);
                  inst = new(ctx) ir_assignment(col_ref, rhs);
               for (unsigned i = cols_to_init; i < type->matrix_columns; i++) {
      ir_constant *const col_idx = new(ctx) ir_constant(i);
                  ir_rvalue *const rhs_ref = new(ctx) ir_dereference_variable(rhs_var);
                  inst = new(ctx) ir_assignment(col_ref, rhs);
         } else if (first_param->type->is_matrix()) {
      /* From page 50 (56 of the PDF) of the GLSL 1.50 spec:
   *
   *     "If a matrix is constructed from a matrix, then each component
   *     (column i, row j) in the result that has a corresponding
   *     component (column i, row j) in the argument will be initialized
   *     from there. All other components will be initialized to the
   *     identity matrix. If a matrix argument is given to a matrix
   *     constructor, it is an error to have any other arguments."
   */
   assert(first_param->next->is_tail_sentinel());
            /* If the source matrix is smaller, pre-initialize the relavent parts of
   * the destination matrix to the identity matrix.
   */
   if ((src_matrix->type->matrix_columns < var->type->matrix_columns) ||
               /* If the source matrix has fewer rows, every column of the
   * destination must be initialized.  Otherwise only the columns in
   * the destination that do not exist in the source must be
   * initialized.
   */
   unsigned col =
                  const glsl_type *const col_type = var->type->column_type();
                     if (!col_type->is_double()) {
      ident.f[0] = 0.0f;
   ident.f[1] = 0.0f;
   ident.f[2] = 0.0f;
   ident.f[3] = 0.0f;
      } else {
      ident.d[0] = 0.0;
   ident.d[1] = 0.0;
   ident.d[2] = 0.0;
                                             ir_instruction *inst = new(ctx) ir_assignment(lhs, rhs);
                  /* Assign columns from the source matrix to the destination matrix.
   *
   * Since the parameter will be used in the RHS of multiple assignments,
   * generate a temporary and copy the paramter there.
   */
   ir_variable *const rhs_var =
      new(ctx) ir_variable(first_param->type, "mat_ctor_mat",
               ir_dereference *const rhs_var_ref =
         ir_instruction *const inst =
                  const unsigned last_row = MIN2(src_matrix->type->vector_elements,
         const unsigned last_col = MIN2(src_matrix->type->matrix_columns,
            unsigned swiz[4] = { 0, 0, 0, 0 };
   for (unsigned i = 1; i < last_row; i++)
                     for (unsigned i = 0; i < last_col; i++) {
      ir_dereference *const lhs =
                        /* If one matrix has columns that are smaller than the columns of the
   * other matrix, wrap the column access of the larger with a swizzle
   * so that the LHS and RHS of the assignment have the same size (and
   * therefore have the same type).
   *
   * It would be perfectly valid to unconditionally generate the
   * swizzles, this this will typically result in a more compact IR
   * tree.
   */
   ir_rvalue *rhs;
   if (lhs->type->vector_elements != rhs_col->type->vector_elements) {
         } else {
                  ir_instruction *inst =
               } else {
      const unsigned cols = type->matrix_columns;
   const unsigned rows = type->vector_elements;
   unsigned remaining_slots = rows * cols;
   unsigned col_idx = 0;
            foreach_in_list(ir_rvalue, rhs, parameters) {
                                    /* Since the parameter might be used in the RHS of two assignments,
   * generate a temporary and copy the paramter there.
   */
   ir_variable *rhs_var =
                  ir_dereference *rhs_var_ref =
                        do {
      /* Assign the current parameter to as many components of the matrix
   * as it will fill.
   *
   * NOTE: A single vector parameter can span two matrix columns.  A
   * single vec4, for example, can completely fill a mat2.
   */
                  rhs_var_ref = new(ctx) ir_dereference_variable(rhs_var);
   ir_instruction *inst = assign_to_matrix_column(var, col_idx,
                           instructions->push_tail(inst);
   rhs_base += count;
                  /* Sometimes, there is still data left in the parameters and
   * components left to be set in the destination but in other
   * column.
   */
   if (row_idx >= rows) {
      row_idx = 0;
                           }
         static ir_rvalue *
   emit_inline_record_constructor(const glsl_type *type,
                     {
      ir_variable *const var =
         ir_dereference_variable *const d =
                     exec_node *node = parameters->get_head_raw();
   for (unsigned i = 0; i < type->length; i++) {
               ir_dereference *const lhs =
                  ir_rvalue *const rhs = ((ir_instruction *) node)->as_rvalue();
                     instructions->push_tail(assign);
                  }
         static ir_rvalue *
   process_record_constructor(exec_list *instructions,
                     {
      void *ctx = state;
   /* From page 32 (page 38 of the PDF) of the GLSL 1.20 spec:
   *
   *    "The arguments to the constructor will be used to set the structure's
   *     fields, in order, using one argument per field. Each argument must
   *     be the same type as the field it sets, or be a type that can be
   *     converted to the field's type according to Section 4.1.10 “Implicit
   *     Conversions.”"
   *
   * From page 35 (page 41 of the PDF) of the GLSL 4.20 spec:
   *
   *    "In all cases, the innermost initializer (i.e., not a list of
   *     initializers enclosed in curly braces) applied to an object must
   *     have the same type as the object being initialized or be a type that
   *     can be converted to the object's type according to section 4.1.10
   *     "Implicit Conversions". In the latter case, an implicit conversion
   *     will be done on the initializer before the assignment is done."
   */
            const unsigned parameter_count =
                  if (parameter_count != constructor_type->length) {
      _mesa_glsl_error(loc, state,
                  "%s parameters in constructor for `%s'",
                           int i = 0;
   /* Type cast each parameter and, if possible, fold constants. */
               const glsl_struct_field *struct_field =
            /* Apply implicit conversions (not the scalar constructor rules, see the
   * spec quote above!) and attempt to convert the parameter to a constant
   * valued expression. After doing so, track whether or not all the
   * parameters to the constructor are trivially constant valued
   * expressions.
   */
   all_parameters_are_constant &=
                  if (ir->type != struct_field->type) {
      _mesa_glsl_error(loc, state,
                  "parameter type mismatch in constructor for `%s.%s' "
   "(%s vs %s)",
   glsl_get_type_name(constructor_type),
                              if (all_parameters_are_constant) {
         } else {
      return emit_inline_record_constructor(constructor_type, instructions,
         }
      ir_rvalue *
   ast_function_expression::handle_method(exec_list *instructions,
         {
      const ast_expression *field = subexpressions[0];
   ir_rvalue *op;
   ir_rvalue *result;
   void *ctx = state;
   /* Handle "method calls" in GLSL 1.20 - namely, array.length() */
   YYLTYPE loc = get_location();
            const char *method;
            /* This would prevent to raise "uninitialized variable" warnings when
   * calling array.length.
   */
   field->subexpressions[0]->set_is_lhs(true);
   op = field->subexpressions[0]->hir(instructions, state);
   if (strcmp(method, "length") == 0) {
      if (!this->expressions.is_empty()) {
      _mesa_glsl_error(&loc, state, "length method takes no arguments");
               if (op->type->is_array()) {
      if (op->type->is_unsized_array()) {
      if (!state->has_shader_storage_buffer_objects()) {
      _mesa_glsl_error(&loc, state,
                        } else if (op->variable_referenced()->is_in_shader_storage_block()) {
      /* Calculate length of an unsized array in run-time */
   result = new(ctx)
      } else {
      /* When actual size is known at link-time, this will be
   * replaced with a constant expression.
   */
   result = new (ctx)
         } else {
            } else if (op->type->is_vector()) {
      if (state->has_420pack()) {
      /* .length() returns int. */
      } else {
      _mesa_glsl_error(&loc, state, "length method on matrix only"
               } else if (op->type->is_matrix()) {
      if (state->has_420pack()) {
      /* .length() returns int. */
      } else {
      _mesa_glsl_error(&loc, state, "length method on matrix only"
               } else {
      _mesa_glsl_error(&loc, state, "length called on scalar.");
         } else {
      _mesa_glsl_error(&loc, state, "unknown method: `%s'", method);
      }
      fail:
         }
      static inline bool is_valid_constructor(const glsl_type *type,
         {
      return type->is_numeric() || type->is_boolean() ||
      }
      ir_rvalue *
   ast_function_expression::hir(exec_list *instructions,
         {
      void *ctx = state;
   /* There are three sorts of function calls.
   *
   * 1. constructors - The first subexpression is an ast_type_specifier.
   * 2. methods - Only the .length() method of array types.
   * 3. functions - Calls to regular old functions.
   *
   */
   if (is_constructor()) {
      const ast_type_specifier *type =
         YYLTYPE loc = type->get_location();
                     /* constructor_type can be NULL if a variable with the same name as the
   * structure has come into scope.
   */
   if (constructor_type == NULL) {
      _mesa_glsl_error(& loc, state, "unknown type `%s' (structure name "
                              /* Constructors for opaque types are illegal.
   *
   * From section 4.1.7 of the ARB_bindless_texture spec:
   *
   * "Samplers are represented using 64-bit integer handles, and may be "
   *  converted to and from 64-bit integers using constructors."
   *
   * From section 4.1.X of the ARB_bindless_texture spec:
   *
   * "Images are represented using 64-bit integer handles, and may be
   *  converted to and from 64-bit integers using constructors."
   */
   if (constructor_type->contains_atomic() ||
      (!state->has_bindless() && constructor_type->contains_opaque())) {
   _mesa_glsl_error(& loc, state, "cannot construct %s type `%s'",
                           if (constructor_type->is_subroutine()) {
      _mesa_glsl_error(& loc, state,
                           if (constructor_type->is_array()) {
      if (!state->check_version(state->allow_glsl_120_subset_in_110 ? 110 : 120,
                        return process_array_constructor(instructions, constructor_type,
                  /* There are two kinds of constructor calls.  Constructors for arrays and
   * structures must have the exact number of arguments with matching types
   * in the correct order.  These constructors follow essentially the same
   * type matching rules as functions.
   *
   * Constructors for built-in language types, such as mat4 and vec2, are
   * free form.  The only requirements are that the parameters must provide
   * enough values of the correct scalar type and that no arguments are
   * given past the last used argument.
   *
   * When using the C-style initializer syntax from GLSL 4.20, constructors
   * must have the exact number of arguments with matching types in the
   * correct order.
   */
   if (constructor_type->is_struct()) {
      return process_record_constructor(instructions, constructor_type,
                     if (!is_valid_constructor(constructor_type, state))
            /* Total number of components of the type being constructed. */
            /* Number of components from parameters that have actually been
   * consumed.  This is used to perform several kinds of error checking.
   */
            unsigned matrix_parameters = 0;
   unsigned nonmatrix_parameters = 0;
            foreach_list_typed(ast_node, ast, link, &this->expressions) {
               /* From page 50 (page 56 of the PDF) of the GLSL 1.50 spec:
   *
   *    "It is an error to provide extra arguments beyond this
   *    last used argument."
   */
   if (components_used >= type_components) {
      _mesa_glsl_error(& loc, state, "too many parameters to `%s' "
                           if (!is_valid_constructor(result->type, state)) {
      _mesa_glsl_error(& loc, state, "cannot construct `%s' from a "
                           /* Count the number of matrix and nonmatrix parameters.  This
   * is used below to enforce some of the constructor rules.
   */
   if (result->type->is_matrix())
                        actual_parameters.push_tail(result);
               /* From page 28 (page 34 of the PDF) of the GLSL 1.10 spec:
   *
   *    "It is an error to construct matrices from other matrices. This
   *    is reserved for future use."
   */
   if (matrix_parameters > 0
      && constructor_type->is_matrix()
   && !state->check_version(120, 100, &loc,
                           /* From page 50 (page 56 of the PDF) of the GLSL 1.50 spec:
   *
   *    "If a matrix argument is given to a matrix constructor, it is
   *    an error to have any other arguments."
   */
   if ((matrix_parameters > 0)
      && ((matrix_parameters + nonmatrix_parameters) > 1)
   && constructor_type->is_matrix()) {
   _mesa_glsl_error(& loc, state, "for matrix `%s' constructor, "
                           /* From page 28 (page 34 of the PDF) of the GLSL 1.10 spec:
   *
   *    "In these cases, there must be enough components provided in the
   *    arguments to provide an initializer for every component in the
   *    constructed value."
   */
   if (components_used < type_components && components_used != 1
      && matrix_parameters == 0) {
   _mesa_glsl_error(& loc, state, "too few components to construct "
                           /* Matrices can never be consumed as is by any constructor but matrix
   * constructors. If the constructor type is not matrix, always break the
   * matrix up into a series of column vectors.
   */
   if (!constructor_type->is_matrix()) {
      foreach_in_list_safe(ir_rvalue, matrix, &actual_parameters) {
                     /* Create a temporary containing the matrix. */
   ir_variable *var = new(ctx) ir_variable(matrix->type, "matrix_tmp",
         instructions->push_tail(var);
   instructions->push_tail(
                        /* Replace the matrix with dereferences of its columns. */
   for (int i = 0; i < matrix->type->matrix_columns; i++) {
      matrix->insert_before(
      new (ctx) ir_dereference_array(var,
   }
                           /* Type cast each parameter and, if possible, fold constants.*/
   foreach_in_list_safe(ir_rvalue, ir, &actual_parameters) {
               /* From section 5.4.1 of the ARB_bindless_texture spec:
   *
   * "In the following four constructors, the low 32 bits of the sampler
   *  type correspond to the .x component of the uvec2 and the high 32
   *  bits correspond to the .y component."
   *
   *  uvec2(any sampler type)     // Converts a sampler type to a
   *                              //   pair of 32-bit unsigned integers
   *  any sampler type(uvec2)     // Converts a pair of 32-bit unsigned integers to
   *                              //   a sampler type
   *  uvec2(any image type)       // Converts an image type to a
   *                              //   pair of 32-bit unsigned integers
   *  any image type(uvec2)       // Converts a pair of 32-bit unsigned integers to
   *                              //   an image type
   */
   if (ir->type->is_sampler() || ir->type->is_image()) {
      /* Convert a sampler/image type to a pair of 32-bit unsigned
   * integers as defined by ARB_bindless_texture.
   */
   if (constructor_type != glsl_type::uvec2_type) {
      _mesa_glsl_error(&loc, state, "sampler and image types can only "
            }
      } else if (constructor_type->is_sampler() ||
            /* Convert a pair of 32-bit unsigned integers to a sampler or image
   * type as defined by ARB_bindless_texture.
   */
   if (ir->type != glsl_type::uvec2_type) {
      _mesa_glsl_error(&loc, state, "sampler and image types can only "
            }
      } else {
      desired_type =
      glsl_type::get_instance(constructor_type->base_type,
                           /* Attempt to convert the parameter to a constant valued expression.
   * After doing so, track whether or not all the parameters to the
   * constructor are trivially constant valued expressions.
                  if (constant != NULL)
                        if (result != ir) {
                     /* If all of the parameters are trivially constant, create a
   * constant representing the complete collection of parameters.
   */
   if (all_parameters_are_constant) {
         } else if (constructor_type->is_scalar()) {
      return dereference_component((ir_rvalue *)
            } else if (constructor_type->is_vector()) {
      return emit_inline_vector_constructor(constructor_type,
                  } else {
      assert(constructor_type->is_matrix());
   return emit_inline_matrix_constructor(constructor_type,
                     } else if (subexpressions[0]->oper == ast_field_selection) {
         } else {
      const ast_expression *id = subexpressions[0];
   const char *func_name = NULL;
   YYLTYPE loc = get_location();
   exec_list actual_parameters;
   ir_variable *sub_var = NULL;
            process_parameters(instructions, &actual_parameters, &this->expressions,
            if (id->oper == ast_array_index) {
      array_idx = generate_array_index(ctx, instructions, state, loc,
                  } else if (id->oper == ast_identifier) {
         } else {
                  /* an error was emitted earlier */
   if (!func_name)
            ir_function_signature *sig =
            ir_rvalue *value = NULL;
   if (sig == NULL) {
      sig = match_subroutine_by_name(func_name, &actual_parameters,
               if (sig == NULL) {
      no_matching_function_error(func_name, &loc,
            } else if (!verify_parameter_modes(state, sig,
                  /* an error has already been emitted */
      } else if (sig->is_builtin() && strcmp(func_name, "ftransform") == 0) {
      /* ftransform refers to global variables, and we don't have any code
   * for remapping the variable references in the built-in shader.
   */
   ir_variable *mvp =
         ir_variable *vtx = state->symbols->get_variable("gl_Vertex");
   value = new(ctx) ir_expression(ir_binop_mul, glsl_type::vec4_type,
            } else {
      bool is_begin_interlock = false;
   bool is_end_interlock = false;
   if (sig->is_builtin() &&
      state->stage == MESA_SHADER_FRAGMENT &&
   state->ARB_fragment_shader_interlock_enable) {
   is_begin_interlock = strcmp(func_name, "beginInvocationInterlockARB") == 0;
               if (sig->is_builtin() &&
      ((state->stage == MESA_SHADER_TESS_CTRL &&
         is_begin_interlock || is_end_interlock)) {
   if (state->current_function == NULL ||
      strcmp(state->current_function->function_name(), "main") != 0) {
                     if (state->found_return) {
                        if (instructions != &state->current_function->body) {
      _mesa_glsl_error(&loc, state,
                  /* There can be only one begin/end interlock pair in the function. */
   if (is_begin_interlock) {
      if (state->found_begin_interlock)
      _mesa_glsl_error(&loc, state,
         } else if (is_end_interlock) {
      if (!state->found_begin_interlock)
      _mesa_glsl_error(&loc, state,
            if (state->found_end_interlock)
      _mesa_glsl_error(&loc, state,
                  value = generate_call(instructions, sig, &actual_parameters, sub_var,
         if (!value) {
      ir_variable *const tmp = new(ctx) ir_variable(glsl_type::void_type,
               instructions->push_tail(tmp);
                                 }
      bool
   ast_function_expression::has_sequence_subexpression() const
   {
      foreach_list_typed(const ast_node, ast, link, &this->expressions) {
      if (ast->has_sequence_subexpression())
                  }
      ir_rvalue *
   ast_aggregate_initializer::hir(exec_list *instructions,
         {
      void *ctx = state;
            if (!this->constructor_type) {
      _mesa_glsl_error(&loc, state, "type of C-style initializer unknown");
      }
            if (!state->has_420pack()) {
      _mesa_glsl_error(&loc, state, "C-style initialization requires the "
                     if (constructor_type->is_array()) {
      return process_array_constructor(instructions, constructor_type, &loc,
               if (constructor_type->is_struct()) {
      return process_record_constructor(instructions, constructor_type, &loc,
               return process_vec_mat_constructor(instructions, constructor_type, &loc,
      }
