   /*
   * Copyright © 2014 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   * Authors:
   *    Connor Abbott (cwabbott0@gmail.com)
   *
   */
      #include "float64_glsl.h"
   #include "glsl_to_nir.h"
   #include "ir_visitor.h"
   #include "ir_hierarchical_visitor.h"
   #include "ir.h"
   #include "ir_optimization.h"
   #include "program.h"
   #include "compiler/nir/nir_control_flow.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_builtin_builder.h"
   #include "compiler/nir/nir_deref.h"
   #include "main/errors.h"
   #include "main/mtypes.h"
   #include "main/shaderobj.h"
   #include "util/u_math.h"
   #include "util/perf/cpu_trace.h"
      /*
   * pass to lower GLSL IR to NIR
   *
   * This will lower variable dereferences to loads/stores of corresponding
   * variables in NIR - the variables will be converted to registers in a later
   * pass.
   */
      namespace {
      class nir_visitor : public ir_visitor
   {
   public:
      nir_visitor(const struct gl_constants *consts, nir_shader *shader);
            virtual void visit(ir_variable *);
   virtual void visit(ir_function *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_if *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_demote *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_return *);
   virtual void visit(ir_call *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_emit_vertex *);
   virtual void visit(ir_end_primitive *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_dereference_variable *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_dereference_array *);
                  private:
      void add_instr(nir_instr *instr, unsigned num_components, unsigned bit_size);
            nir_alu_instr *emit(nir_op op, unsigned dest_size, nir_def **srcs);
   nir_alu_instr *emit(nir_op op, unsigned dest_size, nir_def *src1);
   nir_alu_instr *emit(nir_op op, unsigned dest_size, nir_def *src1,
         nir_alu_instr *emit(nir_op op, unsigned dest_size, nir_def *src1,
                     nir_shader *shader;
   nir_function_impl *impl;
   nir_builder b;
                              /* most recent deref instruction created */
            /* whether the IR we're operating on is per-function or global */
                     /* map of ir_variable -> nir_variable */
            /* map of ir_function_signature -> nir_function_overload */
            /* set of nir_variable hold sparse result */
            void adjust_sparse_variable(nir_deref_instr *var_deref, const glsl_type *type,
               };
      /*
   * This visitor runs before the main visitor, calling create_function() for
   * each function so that the main visitor can resolve forward references in
   * calls.
   */
      class nir_function_visitor : public ir_hierarchical_visitor
   {
   public:
      nir_function_visitor(nir_visitor *v) : visitor(v)
   {
   }
         private:
         };
      /* glsl_to_nir can only handle converting certain function paramaters
   * to NIR. This visitor checks for parameters it can't currently handle.
   */
   class ir_function_param_visitor : public ir_hierarchical_visitor
   {
   public:
      ir_function_param_visitor()
         {
            virtual ir_visitor_status visit_enter(ir_function_signature *ir)
               if (ir->is_intrinsic())
            foreach_in_list(ir_variable, param, &ir->parameters) {
      if (!glsl_type_is_vector_or_scalar(param->type)) {
      unsupported = true;
               if (param->data.mode == ir_var_function_inout) {
      unsupported = true;
               if (param->data.mode != ir_var_function_in &&
                  /* SSBO and shared vars might be passed to a built-in such as an
   * atomic memory function, where copying these to a temp before
   * passing to the atomic function is not valid so we must replace
   * these instead. Also, shader inputs for interpolateAt functions
   * also need to be replaced.
   *
   * We have no way to handle this in NIR or the glsl to nir pass
   * currently so let the GLSL IR lowering handle it.
   */
   if (ir->is_builtin()) {
      unsupported = true;
               /* For opaque types, we want the inlined variable references
   * referencing the passed in variable, since that will have
   * the location information, which an assignment of an opaque
   * variable wouldn't.
   *
   * We have no way to handle this in NIR or the glsl to nir pass
   * currently so let the GLSL IR lowering handle it.
   */
   if (param->type->contains_opaque()) {
      unsupported = true;
                  if (!glsl_type_is_vector_or_scalar(ir->return_type) &&
      !ir->return_type->is_void()) {
   unsupported = true;
                              };
      } /* end of anonymous namespace */
         static bool
   has_unsupported_function_param(exec_list *ir)
   {
      ir_function_param_visitor visitor;
   visit_list_elements(&visitor, ir);
      }
      nir_shader *
   glsl_to_nir(const struct gl_constants *consts,
               const struct gl_shader_program *shader_prog,
   {
               const struct gl_shader_compiler_options *gl_options =
                     /* NIR cannot handle instructions after a break so we use the GLSL IR do
   * lower jumps pass to clean those up for now.
   */
   do_lower_jumps(sh->ir, true, true, gl_options->EmitNoMainReturn,
            /* glsl_to_nir can only handle converting certain function paramaters
   * to NIR. If we find something we can't handle then we get the GLSL IR
   * opts to remove it before we continue on.
   *
   * TODO: add missing glsl ir to nir support and remove this loop.
   */
   while (has_unsupported_function_param(sh->ir)) {
                  nir_shader *shader = nir_shader_create(NULL, stage, options,
            nir_visitor v1(consts, shader);
   nir_function_visitor v2(&v1);
   v2.run(sh->ir);
            /* The GLSL IR won't be needed anymore. */
   ralloc_free(sh->ir);
            nir_validate_shader(shader, "after glsl to nir, before function inline");
   if (should_print_nir(shader)) {
      printf("glsl_to_nir\n");
               /* We have to lower away local constant initializers right before we
   * inline functions.  That way they get properly initialized at the top
   * of the function and not at the top of its caller.
   */
   NIR_PASS_V(shader, nir_lower_variable_initializers, nir_var_all);
   NIR_PASS_V(shader, nir_lower_returns);
   NIR_PASS_V(shader, nir_inline_functions);
                     /* We set func->is_entrypoint after nir_function_create if the function
   * is named "main", so we can use nir_remove_non_entrypoints() for this.
   * Now that we have inlined everything remove all of the functions except
   * func->is_entrypoint.
   */
            shader->info.name = ralloc_asprintf(shader, "GLSL%d", shader_prog->Name);
   if (shader_prog->Label)
                     if (shader->info.stage == MESA_SHADER_FRAGMENT) {
      shader->info.fs.pixel_center_integer = sh->Program->info.fs.pixel_center_integer;
   shader->info.fs.origin_upper_left = sh->Program->info.fs.origin_upper_left;
            nir_foreach_variable_in_shader(var, shader) {
      if (var->data.mode == nir_var_system_value &&
      (var->data.location == SYSTEM_VALUE_SAMPLE_ID ||
                              if (var->data.mode == nir_var_shader_out && var->data.fb_fetch_output)
                     }
      nir_visitor::nir_visitor(const struct gl_constants *consts, nir_shader *shader)
   {
      this->consts = consts;
   this->supports_std430 = consts->UseSTD430AsDefaultPacking;
   this->shader = shader;
   this->is_global = true;
   this->var_table = _mesa_pointer_hash_table_create(NULL);
   this->overload_table = _mesa_pointer_hash_table_create(NULL);
   this->sparse_variable_set = _mesa_pointer_set_create(NULL);
   this->result = NULL;
   this->impl = NULL;
   this->deref = NULL;
   this->sig = NULL;
      }
      nir_visitor::~nir_visitor()
   {
      _mesa_hash_table_destroy(this->var_table, NULL);
   _mesa_hash_table_destroy(this->overload_table, NULL);
      }
      nir_deref_instr *
   nir_visitor::evaluate_deref(ir_instruction *ir)
   {
      ir->accept(this);
      }
      nir_constant *
   nir_visitor::constant_copy(ir_constant *ir, void *mem_ctx)
   {
      if (ir == NULL)
                     const unsigned rows = ir->type->vector_elements;
   const unsigned cols = ir->type->matrix_columns;
            ret->num_elements = 0;
   switch (ir->type->base_type) {
   case GLSL_TYPE_UINT:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
                  case GLSL_TYPE_UINT16:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
               case GLSL_TYPE_INT:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
                  case GLSL_TYPE_INT16:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
               case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
      if (cols > 1) {
      ret->elements = ralloc_array(mem_ctx, nir_constant *, cols);
   ret->num_elements = cols;
   for (unsigned c = 0; c < cols; c++) {
      nir_constant *col_const = rzalloc(mem_ctx, nir_constant);
   col_const->num_elements = 0;
   switch (ir->type->base_type) {
   case GLSL_TYPE_FLOAT:
                        case GLSL_TYPE_FLOAT16:
                        case GLSL_TYPE_DOUBLE:
                        default:
         }
         } else {
      switch (ir->type->base_type) {
   case GLSL_TYPE_FLOAT:
      for (unsigned r = 0; r < rows; r++)
               case GLSL_TYPE_FLOAT16:
      for (unsigned r = 0; r < rows; r++)
               case GLSL_TYPE_DOUBLE:
      for (unsigned r = 0; r < rows; r++)
               default:
            }
         case GLSL_TYPE_UINT64:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
               case GLSL_TYPE_INT64:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
               case GLSL_TYPE_BOOL:
      /* Only float base types can be matrices. */
            for (unsigned r = 0; r < rows; r++)
                  case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_ARRAY:
      ret->elements = ralloc_array(mem_ctx, nir_constant *,
                  for (i = 0; i < ir->type->length; i++)
               default:
                     }
      void
   nir_visitor::adjust_sparse_variable(nir_deref_instr *var_deref, const glsl_type *type,
         {
      const glsl_type *texel_type = type->field_type("texel");
            assert(var_deref->deref_type == nir_deref_type_var);
            /* Adjust nir_variable type to align with sparse nir instructions.
   * Because the nir_variable is created with struct type from ir_variable,
   * but sparse nir instructions output with vector dest.
   */
   var->type = glsl_type::get_instance(texel_type->get_base_type()->base_type,
                     /* Record the adjusted variable. */
      }
      static unsigned
   get_nir_how_declared(unsigned how_declared)
   {
      if (how_declared == ir_var_hidden)
            if (how_declared == ir_var_declared_implicitly)
               }
      void
   nir_visitor::visit(ir_variable *ir)
   {
      /* FINISHME: inout parameters */
            if (ir->data.mode == ir_var_function_out)
            nir_variable *var = rzalloc(shader, nir_variable);
   var->type = ir->type;
            var->data.assigned = ir->data.assigned;
   var->data.read_only = ir->data.read_only;
   var->data.centroid = ir->data.centroid;
   var->data.sample = ir->data.sample;
   var->data.patch = ir->data.patch;
   var->data.how_declared = get_nir_how_declared(ir->data.how_declared);
   var->data.invariant = ir->data.invariant;
   var->data.explicit_invariant = ir->data.explicit_invariant;
   var->data.location = ir->data.location;
   var->data.must_be_shader_input = ir->data.must_be_shader_input;
   var->data.stream = ir->data.stream;
   if (ir->data.stream & (1u << 31))
            var->data.precision = ir->data.precision;
   var->data.explicit_location = ir->data.explicit_location;
   var->data.matrix_layout = ir->data.matrix_layout;
   var->data.from_named_ifc_block = ir->data.from_named_ifc_block;
   var->data.compact = false;
            switch(ir->data.mode) {
   case ir_var_auto:
   case ir_var_temporary:
      if (is_global)
         else
               case ir_var_function_in:
   case ir_var_const_in:
      var->data.mode = nir_var_function_temp;
         case ir_var_shader_in:
      if (shader->info.stage == MESA_SHADER_GEOMETRY &&
      ir->data.location == VARYING_SLOT_PRIMITIVE_ID) {
   /* For whatever reason, GLSL IR makes gl_PrimitiveIDIn an input */
   var->data.location = SYSTEM_VALUE_PRIMITIVE_ID;
      } else {
               if (shader->info.stage == MESA_SHADER_TESS_EVAL &&
      (ir->data.location == VARYING_SLOT_TESS_LEVEL_INNER ||
   ir->data.location == VARYING_SLOT_TESS_LEVEL_OUTER)) {
               if (shader->info.stage > MESA_SHADER_VERTEX &&
      ir->data.location >= VARYING_SLOT_CLIP_DIST0 &&
   ir->data.location <= VARYING_SLOT_CULL_DIST1) {
         }
         case ir_var_shader_out:
      var->data.mode = nir_var_shader_out;
   if (shader->info.stage == MESA_SHADER_TESS_CTRL &&
      (ir->data.location == VARYING_SLOT_TESS_LEVEL_INNER ||
   ir->data.location == VARYING_SLOT_TESS_LEVEL_OUTER)) {
               if (shader->info.stage <= MESA_SHADER_GEOMETRY &&
      ir->data.location >= VARYING_SLOT_CLIP_DIST0 &&
   ir->data.location <= VARYING_SLOT_CULL_DIST1) {
      }
         case ir_var_uniform:
      if (ir->get_interface_type())
         else if (ir->type->contains_image() && !ir->data.bindless)
         else
               case ir_var_shader_storage:
      var->data.mode = nir_var_mem_ssbo;
         case ir_var_system_value:
      var->data.mode = nir_var_system_value;
         case ir_var_shader_shared:
      var->data.mode = nir_var_mem_shared;
         default:
                  unsigned mem_access = 0;
   if (ir->data.memory_read_only)
         if (ir->data.memory_write_only)
         if (ir->data.memory_coherent)
         if (ir->data.memory_volatile)
         if (ir->data.memory_restrict)
                     /* For UBO and SSBO variables, we need explicit types */
   if (var->data.mode & (nir_var_mem_ubo | nir_var_mem_ssbo)) {
      const glsl_type *explicit_ifc_type =
                     if (ir->type->without_array()->is_interface()) {
      /* If the type contains the interface, wrap the explicit type in the
   * right number of arrays.
   */
      } else {
      /* Otherwise, this variable is one entry in the interface */
   UNUSED bool found = false;
   for (unsigned i = 0; i < explicit_ifc_type->length; i++) {
      const glsl_struct_field *field =
                        var->type = field->type;
   if (field->memory_read_only)
         if (field->memory_write_only)
         if (field->memory_coherent)
         if (field->memory_volatile)
                        found = true;
      }
                  var->data.interpolation = ir->data.interpolation;
            switch (ir->data.depth_layout) {
   case ir_depth_layout_none:
      var->data.depth_layout = nir_depth_layout_none;
      case ir_depth_layout_any:
      var->data.depth_layout = nir_depth_layout_any;
      case ir_depth_layout_greater:
      var->data.depth_layout = nir_depth_layout_greater;
      case ir_depth_layout_less:
      var->data.depth_layout = nir_depth_layout_less;
      case ir_depth_layout_unchanged:
      var->data.depth_layout = nir_depth_layout_unchanged;
      default:
                  var->data.index = ir->data.index;
   var->data.descriptor_set = 0;
   var->data.binding = ir->data.binding;
   var->data.explicit_binding = ir->data.explicit_binding;
   var->data.explicit_offset = ir->data.explicit_xfb_offset;
   var->data.bindless = ir->data.bindless;
   var->data.offset = ir->data.offset;
            if (var->type->without_array()->is_image()) {
         } else if (var->data.mode == nir_var_shader_out) {
      var->data.xfb.buffer = ir->data.xfb_buffer;
               var->data.fb_fetch_output = ir->data.fb_fetch_output;
   var->data.explicit_xfb_buffer = ir->data.explicit_xfb_buffer;
            var->num_state_slots = ir->get_num_state_slots();
   if (var->num_state_slots > 0) {
      var->state_slots = rzalloc_array(var, nir_state_slot,
            ir_state_slot *state_slots = ir->get_state_slots();
   for (unsigned i = 0; i < var->num_state_slots; i++) {
      for (unsigned j = 0; j < 4; j++)
         } else {
                  /* Values declared const will have ir->constant_value instead of
   * ir->constant_initializer.
   */
   if (ir->constant_initializer)
         else
            if (var->data.mode == nir_var_function_temp)
         else
               }
      ir_visitor_status
   nir_function_visitor::visit_enter(ir_function *ir)
   {
      foreach_in_list(ir_function_signature, sig, &ir->signatures) {
         }
      }
      void
   nir_visitor::create_function(ir_function_signature *ir)
   {
      if (ir->is_intrinsic())
            nir_function *func = nir_function_create(shader, ir->function_name());
   if (strcmp(ir->function_name(), "main") == 0)
            func->num_params = ir->parameters.length() +
                           if (ir->return_type != glsl_type::void_type) {
      /* The return value is a variable deref (basically an out parameter) */
   func->params[np].num_components = 1;
   func->params[np].bit_size = 32;
               foreach_in_list(ir_variable, param, &ir->parameters) {
      /* FINISHME: pass arrays, structs, etc by reference? */
            if (param->data.mode == ir_var_function_in) {
      func->params[np].num_components = param->type->vector_elements;
      } else {
      func->params[np].num_components = 1;
      }
      }
               }
      void
   nir_visitor::visit(ir_function *ir)
   {
      foreach_in_list(ir_function_signature, sig, &ir->signatures)
      }
      void
   nir_visitor::visit(ir_function_signature *ir)
   {
      if (ir->is_intrinsic())
                     struct hash_entry *entry =
            assert(entry);
            if (ir->is_defined) {
      nir_function_impl *impl = nir_function_impl_create(func);
                                       foreach_in_list(ir_variable, param, &ir->parameters) {
                     if (param->data.mode == ir_var_function_in) {
                  _mesa_hash_table_insert(var_table, param, var);
                           } else {
            }
      void
   nir_visitor::visit(ir_loop *ir)
   {
      nir_push_loop(&b);
   visit_exec_list(&ir->body_instructions, this);
      }
      void
   nir_visitor::visit(ir_if *ir)
   {
      nir_push_if(&b, evaluate_rvalue(ir->condition));
   visit_exec_list(&ir->then_instructions, this);
   nir_push_else(&b, NULL);
   visit_exec_list(&ir->else_instructions, this);
      }
      void
   nir_visitor::visit(ir_discard *ir)
   {
      /*
   * discards aren't treated as control flow, because before we lower them
   * they can appear anywhere in the shader and the stuff after them may still
   * be executed (yay, crazy GLSL rules!). However, after lowering, all the
   * discards will be immediately followed by a return.
            if (ir->condition)
         else
      }
      void
   nir_visitor::visit(ir_demote *ir)
   {
         }
      void
   nir_visitor::visit(ir_emit_vertex *ir)
   {
         }
      void
   nir_visitor::visit(ir_end_primitive *ir)
   {
         }
      void
   nir_visitor::visit(ir_loop_jump *ir)
   {
      nir_jump_type type;
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      type = nir_jump_break;
      case ir_loop_jump::jump_continue:
      type = nir_jump_continue;
      default:
                  nir_jump_instr *instr = nir_jump_instr_create(this->shader, type);
      }
      void
   nir_visitor::visit(ir_return *ir)
   {
      if (ir->value != NULL) {
      nir_deref_instr *ret_deref =
                  nir_def *val = evaluate_rvalue(ir->value);
               nir_jump_instr *instr = nir_jump_instr_create(this->shader, nir_jump_return);
      }
      static void
   intrinsic_set_std430_align(nir_intrinsic_instr *intrin, const glsl_type *type)
   {
      unsigned bit_size = type->is_boolean() ? 32 : glsl_get_bit_size(type);
   unsigned pow2_components = util_next_power_of_two(type->vector_elements);
      }
      /* Accumulate any qualifiers along the deref chain to get the actual
   * load/store qualifier.
   */
      static enum gl_access_qualifier
   deref_get_qualifier(nir_deref_instr *deref)
   {
      nir_deref_path path;
                     const glsl_type *parent_type = path.path[0]->type;
   for (nir_deref_instr **cur_ptr = &path.path[1]; *cur_ptr; cur_ptr++) {
               if (parent_type->is_interface()) {
      const struct glsl_struct_field *field =
         if (field->memory_read_only)
         if (field->memory_write_only)
         if (field->memory_coherent)
         if (field->memory_volatile)
         if (field->memory_restrict)
                                       }
      void
   nir_visitor::visit(ir_call *ir)
   {
      if (ir->callee->is_intrinsic()) {
               /* Initialize to something because gcc complains otherwise */
            switch (ir->callee->intrinsic_id) {
   case ir_intrinsic_generic_atomic_add:
      op = nir_intrinsic_deref_atomic;
   atomic_op = ir->return_deref->type->is_integer_32_64()
            case ir_intrinsic_generic_atomic_and:
      op = nir_intrinsic_deref_atomic;
   atomic_op = nir_atomic_op_iand;
      case ir_intrinsic_generic_atomic_or:
      op = nir_intrinsic_deref_atomic;
   atomic_op = nir_atomic_op_ior;
      case ir_intrinsic_generic_atomic_xor:
      op = nir_intrinsic_deref_atomic;
   atomic_op = nir_atomic_op_ixor;
      case ir_intrinsic_generic_atomic_min:
      assert(ir->return_deref);
   op = nir_intrinsic_deref_atomic;
   if (ir->return_deref->type == glsl_type::int_type ||
      ir->return_deref->type == glsl_type::int64_t_type)
      else if (ir->return_deref->type == glsl_type::uint_type ||
               else if (ir->return_deref->type == glsl_type::float_type)
         else
            case ir_intrinsic_generic_atomic_max:
      assert(ir->return_deref);
   op = nir_intrinsic_deref_atomic;
   if (ir->return_deref->type == glsl_type::int_type ||
      ir->return_deref->type == glsl_type::int64_t_type)
      else if (ir->return_deref->type == glsl_type::uint_type ||
               else if (ir->return_deref->type == glsl_type::float_type)
         else
            case ir_intrinsic_generic_atomic_exchange:
      op = nir_intrinsic_deref_atomic;
   atomic_op = nir_atomic_op_xchg;
      case ir_intrinsic_generic_atomic_comp_swap:
      op = nir_intrinsic_deref_atomic_swap;
   atomic_op = ir->return_deref->type->is_integer_32_64()
      ? nir_atomic_op_cmpxchg
         case ir_intrinsic_atomic_counter_read:
      op = nir_intrinsic_atomic_counter_read_deref;
      case ir_intrinsic_atomic_counter_increment:
      op = nir_intrinsic_atomic_counter_inc_deref;
      case ir_intrinsic_atomic_counter_predecrement:
      op = nir_intrinsic_atomic_counter_pre_dec_deref;
      case ir_intrinsic_atomic_counter_add:
      op = nir_intrinsic_atomic_counter_add_deref;
      case ir_intrinsic_atomic_counter_and:
      op = nir_intrinsic_atomic_counter_and_deref;
      case ir_intrinsic_atomic_counter_or:
      op = nir_intrinsic_atomic_counter_or_deref;
      case ir_intrinsic_atomic_counter_xor:
      op = nir_intrinsic_atomic_counter_xor_deref;
      case ir_intrinsic_atomic_counter_min:
      op = nir_intrinsic_atomic_counter_min_deref;
      case ir_intrinsic_atomic_counter_max:
      op = nir_intrinsic_atomic_counter_max_deref;
      case ir_intrinsic_atomic_counter_exchange:
      op = nir_intrinsic_atomic_counter_exchange_deref;
      case ir_intrinsic_atomic_counter_comp_swap:
      op = nir_intrinsic_atomic_counter_comp_swap_deref;
      case ir_intrinsic_image_load:
      op = nir_intrinsic_image_deref_load;
      case ir_intrinsic_image_store:
      op = nir_intrinsic_image_deref_store;
      case ir_intrinsic_image_atomic_add:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = ir->return_deref->type->is_integer_32_64()
      ? nir_atomic_op_iadd
         case ir_intrinsic_image_atomic_min:
      op = nir_intrinsic_image_deref_atomic;
   if (ir->return_deref->type == glsl_type::int_type)
         else if (ir->return_deref->type == glsl_type::uint_type)
         else
            case ir_intrinsic_image_atomic_max:
      op = nir_intrinsic_image_deref_atomic;
   if (ir->return_deref->type == glsl_type::int_type)
         else if (ir->return_deref->type == glsl_type::uint_type)
         else
            case ir_intrinsic_image_atomic_and:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = nir_atomic_op_iand;
      case ir_intrinsic_image_atomic_or:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = nir_atomic_op_ior;
      case ir_intrinsic_image_atomic_xor:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = nir_atomic_op_ixor;
      case ir_intrinsic_image_atomic_exchange:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = nir_atomic_op_xchg;
      case ir_intrinsic_image_atomic_comp_swap:
      op = nir_intrinsic_image_deref_atomic_swap;
   atomic_op = nir_atomic_op_cmpxchg;
      case ir_intrinsic_image_atomic_inc_wrap:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = nir_atomic_op_inc_wrap;
      case ir_intrinsic_image_atomic_dec_wrap:
      op = nir_intrinsic_image_deref_atomic;
   atomic_op = nir_atomic_op_dec_wrap;
      case ir_intrinsic_memory_barrier:
   case ir_intrinsic_memory_barrier_buffer:
   case ir_intrinsic_memory_barrier_image:
   case ir_intrinsic_memory_barrier_shared:
   case ir_intrinsic_memory_barrier_atomic_counter:
   case ir_intrinsic_group_memory_barrier:
      op = nir_intrinsic_barrier;
      case ir_intrinsic_image_size:
      op = nir_intrinsic_image_deref_size;
      case ir_intrinsic_image_samples:
      op = nir_intrinsic_image_deref_samples;
      case ir_intrinsic_image_sparse_load:
      op = nir_intrinsic_image_deref_sparse_load;
      case ir_intrinsic_shader_clock:
      op = nir_intrinsic_shader_clock;
      case ir_intrinsic_begin_invocation_interlock:
      op = nir_intrinsic_begin_invocation_interlock;
      case ir_intrinsic_end_invocation_interlock:
      op = nir_intrinsic_end_invocation_interlock;
      case ir_intrinsic_vote_any:
      op = nir_intrinsic_vote_any;
      case ir_intrinsic_vote_all:
      op = nir_intrinsic_vote_all;
      case ir_intrinsic_vote_eq:
      op = nir_intrinsic_vote_ieq;
      case ir_intrinsic_ballot:
      op = nir_intrinsic_ballot;
      case ir_intrinsic_read_invocation:
      op = nir_intrinsic_read_invocation;
      case ir_intrinsic_read_first_invocation:
      op = nir_intrinsic_read_first_invocation;
      case ir_intrinsic_helper_invocation:
      op = nir_intrinsic_is_helper_invocation;
      case ir_intrinsic_is_sparse_texels_resident:
      op = nir_intrinsic_is_sparse_texels_resident;
      default:
                  nir_intrinsic_instr *instr = nir_intrinsic_instr_create(shader, op);
            switch (op) {
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap: {
                     /* Deref */
   exec_node *param = ir->actual_parameters.get_head();
   ir_rvalue *rvalue = (ir_rvalue *) param;
   ir_dereference *deref = rvalue->as_dereference();
   ir_swizzle *swizzle = NULL;
   if (!deref) {
      /* We may have a swizzle to pick off a single vec4 component */
   swizzle = rvalue->as_swizzle();
   assert(swizzle && swizzle->type->vector_elements == 1);
   deref = swizzle->val->as_dereference();
      }
   nir_deref_instr *nir_deref = evaluate_deref(deref);
   if (swizzle) {
      nir_deref = nir_build_deref_array_imm(&b, nir_deref,
                                    /* data1 parameter (this is always present) */
   param = param->get_next();
                  /* data2 parameter (only with atomic_comp_swap) */
   if (param_count == 3) {
      assert(op == nir_intrinsic_deref_atomic_swap);
   param = param->get_next();
   inst = (ir_instruction *) param;
               /* Atomic result */
   assert(ir->return_deref);
   if (ir->return_deref->type->is_integer_64()) {
      nir_def_init(&instr->instr, &instr->def,
      } else {
      nir_def_init(&instr->instr, &instr->def,
      }
   nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_atomic_counter_read_deref:
   case nir_intrinsic_atomic_counter_inc_deref:
   case nir_intrinsic_atomic_counter_pre_dec_deref:
   case nir_intrinsic_atomic_counter_add_deref:
   case nir_intrinsic_atomic_counter_min_deref:
   case nir_intrinsic_atomic_counter_max_deref:
   case nir_intrinsic_atomic_counter_and_deref:
   case nir_intrinsic_atomic_counter_or_deref:
   case nir_intrinsic_atomic_counter_xor_deref:
   case nir_intrinsic_atomic_counter_exchange_deref:
   case nir_intrinsic_atomic_counter_comp_swap_deref: {
      /* Set the counter variable dereference. */
                                 /* Set the intrinsic destination. */
   if (ir->return_deref) {
                  /* Set the intrinsic parameters. */
   if (!param->is_tail_sentinel()) {
      instr->src[1] =
                     if (!param->is_tail_sentinel()) {
      instr->src[2] =
                     nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_sparse_load: {
      /* Set the image variable dereference. */
   exec_node *param = ir->actual_parameters.get_head();
   ir_dereference *image = (ir_dereference *)param;
                           if (op == nir_intrinsic_image_deref_atomic ||
      op == nir_intrinsic_image_deref_atomic_swap) {
               instr->src[0] = nir_src_for_ssa(&deref->def);
   param = param->get_next();
   nir_intrinsic_set_image_dim(instr,
                  /* Set the intrinsic destination. */
   if (ir->return_deref) {
      unsigned num_components;
   if (op == nir_intrinsic_image_deref_sparse_load) {
      const glsl_type *dest_type =
         /* One extra component to hold residency code. */
                                 if (op == nir_intrinsic_image_deref_size) {
         } else if (op == nir_intrinsic_image_deref_load ||
            instr->num_components = instr->def.num_components;
   nir_intrinsic_set_dest_type(instr,
      } else if (op == nir_intrinsic_image_deref_store) {
      instr->num_components = 4;
   nir_intrinsic_set_src_type(instr,
               if (op == nir_intrinsic_image_deref_size ||
      op == nir_intrinsic_image_deref_samples) {
   /* image_deref_size takes an LOD parameter which is always 0
   * coming from GLSL.
   */
   if (op == nir_intrinsic_image_deref_size)
         nir_builder_instr_insert(&b, &instr->instr);
               /* Set the address argument, extending the coordinate vector to four
   * components.
   */
   nir_def *src_addr =
                  for (int i = 0; i < 4; i++) {
      if (i < type->coordinate_components())
         else
                              /* Set the sample argument, which is undefined for single-sample
   * images.
   */
   if (type->sampler_dimensionality == GLSL_SAMPLER_DIM_MS) {
      instr->src[2] =
            } else {
                  /* Set the intrinsic parameters. */
   if (!param->is_tail_sentinel()) {
      instr->src[3] =
            } else if (op == nir_intrinsic_image_deref_load ||
                        if (!param->is_tail_sentinel()) {
      instr->src[4] =
            } else if (op == nir_intrinsic_image_deref_store) {
                  nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_barrier: {
      /* The nir_intrinsic_barrier follows the general
   * semantics of SPIR-V memory barriers, so this and other memory
   * barriers use the mapping based on GLSL->SPIR-V from
   *
   *   https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_gl_spirv.txt
   */
   mesa_scope scope;
   unsigned modes;
   switch (ir->callee->intrinsic_id) {
   case ir_intrinsic_memory_barrier:
      scope = SCOPE_DEVICE;
   modes = nir_var_image |
         nir_var_mem_ssbo |
   nir_var_mem_shared |
      case ir_intrinsic_memory_barrier_buffer:
      scope = SCOPE_DEVICE;
   modes = nir_var_mem_ssbo |
            case ir_intrinsic_memory_barrier_image:
      scope = SCOPE_DEVICE;
   modes = nir_var_image;
      case ir_intrinsic_memory_barrier_shared:
      /* Both ARB_gl_spirv and glslang lower this to Device scope, so
   * follow their lead.  Note GL_KHR_vulkan_glsl also does
   * something similar.
   */
   scope = SCOPE_DEVICE;
   modes = nir_var_mem_shared;
      case ir_intrinsic_group_memory_barrier:
      scope = SCOPE_WORKGROUP;
   modes = nir_var_image |
         nir_var_mem_ssbo |
   nir_var_mem_shared |
      case ir_intrinsic_memory_barrier_atomic_counter:
      /* There's no nir_var_atomic_counter, but since atomic counters are lowered
   * to SSBOs, we use nir_var_mem_ssbo instead.
   */
   scope = SCOPE_DEVICE;
   modes = nir_var_mem_ssbo;
      default:
                  nir_scoped_memory_barrier(&b, scope, NIR_MEMORY_ACQ_REL,
            }
   case nir_intrinsic_shader_clock:
      nir_def_init(&instr->instr, &instr->def, 2, 32);
   nir_intrinsic_set_memory_scope(instr, SCOPE_SUBGROUP);
   nir_builder_instr_insert(&b, &instr->instr);
      case nir_intrinsic_begin_invocation_interlock:
      nir_builder_instr_insert(&b, &instr->instr);
      case nir_intrinsic_end_invocation_interlock:
      nir_builder_instr_insert(&b, &instr->instr);
      case nir_intrinsic_store_ssbo: {
                                                   param = param->get_next();
                  nir_def *nir_val = evaluate_rvalue(val);
                  instr->src[0] = nir_src_for_ssa(nir_val);
   instr->src[1] = nir_src_for_ssa(evaluate_rvalue(block));
   instr->src[2] = nir_src_for_ssa(evaluate_rvalue(offset));
   intrinsic_set_std430_align(instr, val->type);
                  nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_load_shared: {
                                    const glsl_type *type = ir->return_deref->var->type;
                  /* Setup destination register */
   unsigned bit_size = type->is_boolean() ? 32 : glsl_get_bit_size(type);
                           /* The value in shared memory is a 32-bit value */
   if (type->is_boolean())
            }
   case nir_intrinsic_store_shared: {
                                    param = param->get_next();
                                          nir_def *nir_val = evaluate_rvalue(val);
   /* The value in shared memory is a 32-bit value */
                  instr->src[0] = nir_src_for_ssa(nir_val);
                  nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_vote_ieq:
      instr->num_components = 1;
      case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all: {
                              nir_builder_instr_insert(&b, &instr->instr);
               case nir_intrinsic_ballot: {
      nir_def_init(&instr->instr, &instr->def,
                                 nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_read_invocation: {
      nir_def_init(&instr->instr, &instr->def,
                                                nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_read_first_invocation: {
      nir_def_init(&instr->instr, &instr->def,
                                 nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_is_helper_invocation: {
      nir_def_init(&instr->instr, &instr->def, 1, 1);
   nir_builder_instr_insert(&b, &instr->instr);
      }
   case nir_intrinsic_is_sparse_texels_resident: {
                              nir_builder_instr_insert(&b, &instr->instr);
      }
   default:
                  if (ir->return_deref) {
                                                      struct hash_entry *entry =
         assert(entry);
                     unsigned i = 0;
   nir_deref_instr *ret_deref = NULL;
   if (ir->return_deref) {
      nir_variable *ret_tmp =
      nir_local_variable_create(this->impl, ir->return_deref->type,
      ret_deref = nir_build_deref_var(&b, ret_tmp);
               foreach_two_lists(formal_node, &ir->callee->parameters,
            ir_rvalue *param_rvalue = (ir_rvalue *) actual_node;
            if (sig_param->data.mode == ir_var_function_out) {
      nir_variable *out_param =
         nir_deref_instr *out_param_deref = nir_build_deref_var(&b, out_param);
      } else if (sig_param->data.mode == ir_var_function_in) {
      nir_def *val = evaluate_rvalue(param_rvalue);
      } else if (sig_param->data.mode == ir_var_function_inout) {
                                       /* Copy out params. We must do this after the function call to ensure we
   * do not overwrite global variables prematurely.
   */
   i = ir->return_deref ? 1 : 0;
   foreach_two_lists(formal_node, &ir->callee->parameters,
            ir_rvalue *param_rvalue = (ir_rvalue *) actual_node;
            if (sig_param->data.mode == ir_var_function_out) {
      nir_store_deref(&b, evaluate_deref(param_rvalue),
                                    if (ir->return_deref)
      }
      void
   nir_visitor::visit(ir_assignment *ir)
   {
      unsigned num_components = ir->lhs->type->vector_elements;
            b.exact = ir->lhs->variable_referenced()->data.invariant ||
            if ((ir->rhs->as_dereference() || ir->rhs->as_constant()) &&
      (write_mask == BITFIELD_MASK(num_components) || write_mask == 0)) {
   nir_deref_instr *lhs = evaluate_deref(ir->lhs);
   nir_deref_instr *rhs = evaluate_deref(ir->rhs);
   enum gl_access_qualifier lhs_qualifiers = deref_get_qualifier(lhs);
            nir_copy_deref_with_access(&b, lhs, rhs, lhs_qualifiers,
                     ir_texture *tex = ir->rhs->as_texture();
            if (!is_sparse)
            ir->lhs->accept(this);
   nir_deref_instr *lhs_deref = this->deref;
            if (is_sparse) {
               /* correct component and mask because they are 0 for struct */
   num_components = src->num_components;
               if (write_mask != BITFIELD_MASK(num_components) && write_mask != 0) {
      /* GLSL IR will give us the input to the write-masked assignment in a
   * single packed vector.  So, for example, if the writemask is xzw, then
   * we have to swizzle x -> x, y -> z, and z -> w and get the y component
   * from the load.
   */
   unsigned swiz[4];
   unsigned component = 0;
   for (unsigned i = 0; i < 4; i++) {
         }
                        nir_store_deref_with_access(&b, lhs_deref, src, write_mask,
      }
      /*
   * Given an instruction, returns a pointer to its destination or NULL if there
   * is no destination.
   *
   * Note that this only handles instructions we generate at this level.
   */
   static nir_def *
   get_instr_def(nir_instr *instr)
   {
      nir_alu_instr *alu_instr;
   nir_intrinsic_instr *intrinsic_instr;
            switch (instr->type) {
      case nir_instr_type_alu:
                  case nir_instr_type_intrinsic:
      intrinsic_instr = nir_instr_as_intrinsic(instr);
   if (nir_intrinsic_infos[intrinsic_instr->intrinsic].has_dest)
                     case nir_instr_type_tex:
                  default:
                  }
      void
   nir_visitor::add_instr(nir_instr *instr, unsigned num_components,
         {
               if (def)
                     if (def)
      }
      nir_def *
   nir_visitor::evaluate_rvalue(ir_rvalue* ir)
   {
      ir->accept(this);
   if (ir->as_dereference() || ir->as_constant()) {
      /*
   * A dereference is being used on the right hand side, which means we
   * must emit a variable load.
            enum gl_access_qualifier access = deref_get_qualifier(this->deref);
                  }
      static bool
   type_is_float(glsl_base_type type)
   {
      return type == GLSL_TYPE_FLOAT || type == GLSL_TYPE_DOUBLE ||
      }
      static bool
   type_is_signed(glsl_base_type type)
   {
      return type == GLSL_TYPE_INT || type == GLSL_TYPE_INT64 ||
      }
      void
   nir_visitor::visit(ir_expression *ir)
   {
      /* Some special cases */
   switch (ir->operation) {
   case ir_unop_interpolate_at_centroid:
   case ir_binop_interpolate_at_offset:
   case ir_binop_interpolate_at_sample: {
      ir_dereference *deref = ir->operands[0]->as_dereference();
   ir_swizzle *swizzle = NULL;
   if (!deref) {
      /* the api does not allow a swizzle here, but the varying packing code
   * may have pushed one into here.
   */
   swizzle = ir->operands[0]->as_swizzle();
   assert(swizzle);
   deref = swizzle->val->as_dereference();
                        assert(nir_deref_mode_is(this->deref, nir_var_shader_in));
   nir_intrinsic_op op;
   switch (ir->operation) {
   case ir_unop_interpolate_at_centroid:
      op = nir_intrinsic_interp_deref_at_centroid;
      case ir_binop_interpolate_at_offset:
      op = nir_intrinsic_interp_deref_at_offset;
      case ir_binop_interpolate_at_sample:
      op = nir_intrinsic_interp_deref_at_sample;
      default:
                  nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(shader, op);
   intrin->num_components = deref->type->vector_elements;
            if (intrin->intrinsic == nir_intrinsic_interp_deref_at_offset ||
                  unsigned bit_size =  glsl_get_bit_size(deref->type);
            if (swizzle) {
      unsigned swiz[4] = {
                  result = nir_swizzle(&b, result, swiz,
                           case ir_unop_ssbo_unsized_array_length: {
      nir_intrinsic_instr *intrin =
                  ir_dereference *deref = ir->operands[0]->as_dereference();
            add_instr(&intrin->instr, 1, 32);
               case ir_binop_ubo_load:
      /* UBO loads should only have been lowered in GLSL IR for non-nir drivers,
   * NIR drivers make use of gl_nir_lower_buffers() instead.
   */
      default:
                  nir_def *srcs[4];
   for (unsigned i = 0; i < ir->num_operands; i++)
            glsl_base_type types[4];
   for (unsigned i = 0; i < ir->num_operands; i++)
                     switch (ir->operation) {
   case ir_unop_bit_not: result = nir_inot(&b, srcs[0]); break;
   case ir_unop_logic_not:
      result = nir_inot(&b, srcs[0]);
      case ir_unop_neg:
      result = type_is_float(types[0]) ? nir_fneg(&b, srcs[0])
            case ir_unop_abs:
      result = type_is_float(types[0]) ? nir_fabs(&b, srcs[0])
            case ir_unop_clz:
      result = nir_uclz(&b, srcs[0]);
      case ir_unop_saturate:
      assert(type_is_float(types[0]));
   result = nir_fsat(&b, srcs[0]);
      case ir_unop_sign:
      result = type_is_float(types[0]) ? nir_fsign(&b, srcs[0])
                     case ir_unop_rsq:
      if (consts->ForceGLSLAbsSqrt)
         result = nir_frsq(&b, srcs[0]);
         case ir_unop_sqrt:
      if (consts->ForceGLSLAbsSqrt)
         result = nir_fsqrt(&b, srcs[0]);
         case ir_unop_exp:  result = nir_fexp2(&b, nir_fmul_imm(&b, srcs[0], M_LOG2E)); break;
   case ir_unop_log:  result = nir_fmul_imm(&b, nir_flog2(&b, srcs[0]), 1.0 / M_LOG2E); break;
   case ir_unop_exp2: result = nir_fexp2(&b, srcs[0]); break;
   case ir_unop_log2: result = nir_flog2(&b, srcs[0]); break;
   case ir_unop_i2f:
   case ir_unop_u2f:
   case ir_unop_b2f:
   case ir_unop_f2i:
   case ir_unop_f2u:
   case ir_unop_f2b:
   case ir_unop_i2b:
   case ir_unop_b2i:
   case ir_unop_b2i64:
   case ir_unop_d2f:
   case ir_unop_f2d:
   case ir_unop_f162f:
   case ir_unop_f2f16:
   case ir_unop_f162b:
   case ir_unop_b2f16:
   case ir_unop_i2i:
   case ir_unop_u2u:
   case ir_unop_d2i:
   case ir_unop_d2u:
   case ir_unop_d2b:
   case ir_unop_i2d:
   case ir_unop_u2d:
   case ir_unop_i642i:
   case ir_unop_i642u:
   case ir_unop_i642f:
   case ir_unop_i642b:
   case ir_unop_i642d:
   case ir_unop_u642i:
   case ir_unop_u642u:
   case ir_unop_u642f:
   case ir_unop_u642d:
   case ir_unop_i2i64:
   case ir_unop_u2i64:
   case ir_unop_f2i64:
   case ir_unop_d2i64:
   case ir_unop_i2u64:
   case ir_unop_u2u64:
   case ir_unop_f2u64:
   case ir_unop_d2u64:
   case ir_unop_i2u:
   case ir_unop_u2i:
   case ir_unop_i642u64:
   case ir_unop_u642i64: {
      nir_alu_type src_type = nir_get_nir_type_for_glsl_base_type(types[0]);
   nir_alu_type dst_type = nir_get_nir_type_for_glsl_base_type(out_type);
   result = nir_type_convert(&b, srcs[0], src_type, dst_type,
         /* b2i and b2f don't have fixed bit-size versions so the builder will
   * just assume 32 and we have to fix it up here.
   */
   result->bit_size = nir_alu_type_get_type_size(dst_type);
               case ir_unop_f2fmp: {
      result = nir_build_alu(&b, nir_op_f2fmp, srcs[0], NULL, NULL, NULL);
               case ir_unop_i2imp: {
      result = nir_build_alu(&b, nir_op_i2imp, srcs[0], NULL, NULL, NULL);
               case ir_unop_u2ump: {
      result = nir_build_alu(&b, nir_op_i2imp, srcs[0], NULL, NULL, NULL);
               case ir_unop_bitcast_i2f:
   case ir_unop_bitcast_f2i:
   case ir_unop_bitcast_u2f:
   case ir_unop_bitcast_f2u:
   case ir_unop_bitcast_i642d:
   case ir_unop_bitcast_d2i64:
   case ir_unop_bitcast_u642d:
   case ir_unop_bitcast_d2u64:
   case ir_unop_subroutine_to_int:
      /* no-op */
   result = nir_mov(&b, srcs[0]);
      case ir_unop_trunc: result = nir_ftrunc(&b, srcs[0]); break;
   case ir_unop_ceil:  result = nir_fceil(&b, srcs[0]); break;
   case ir_unop_floor: result = nir_ffloor(&b, srcs[0]); break;
   case ir_unop_fract: result = nir_ffract(&b, srcs[0]); break;
   case ir_unop_frexp_exp: result = nir_frexp_exp(&b, srcs[0]); break;
   case ir_unop_frexp_sig: result = nir_frexp_sig(&b, srcs[0]); break;
   case ir_unop_round_even: result = nir_fround_even(&b, srcs[0]); break;
   case ir_unop_sin:   result = nir_fsin(&b, srcs[0]); break;
   case ir_unop_cos:   result = nir_fcos(&b, srcs[0]); break;
   case ir_unop_dFdx:        result = nir_fddx(&b, srcs[0]); break;
   case ir_unop_dFdy:        result = nir_fddy(&b, srcs[0]); break;
   case ir_unop_dFdx_fine:   result = nir_fddx_fine(&b, srcs[0]); break;
   case ir_unop_dFdy_fine:   result = nir_fddy_fine(&b, srcs[0]); break;
   case ir_unop_dFdx_coarse: result = nir_fddx_coarse(&b, srcs[0]); break;
   case ir_unop_dFdy_coarse: result = nir_fddy_coarse(&b, srcs[0]); break;
   case ir_unop_pack_snorm_2x16:
      result = nir_pack_snorm_2x16(&b, srcs[0]);
      case ir_unop_pack_snorm_4x8:
      result = nir_pack_snorm_4x8(&b, srcs[0]);
      case ir_unop_pack_unorm_2x16:
      result = nir_pack_unorm_2x16(&b, srcs[0]);
      case ir_unop_pack_unorm_4x8:
      result = nir_pack_unorm_4x8(&b, srcs[0]);
      case ir_unop_pack_half_2x16:
      result = nir_pack_half_2x16(&b, srcs[0]);
      case ir_unop_unpack_snorm_2x16:
      result = nir_unpack_snorm_2x16(&b, srcs[0]);
      case ir_unop_unpack_snorm_4x8:
      result = nir_unpack_snorm_4x8(&b, srcs[0]);
      case ir_unop_unpack_unorm_2x16:
      result = nir_unpack_unorm_2x16(&b, srcs[0]);
      case ir_unop_unpack_unorm_4x8:
      result = nir_unpack_unorm_4x8(&b, srcs[0]);
      case ir_unop_unpack_half_2x16:
      result = nir_unpack_half_2x16(&b, srcs[0]);
      case ir_unop_pack_sampler_2x32:
   case ir_unop_pack_image_2x32:
   case ir_unop_pack_double_2x32:
   case ir_unop_pack_int_2x32:
   case ir_unop_pack_uint_2x32:
      result = nir_pack_64_2x32(&b, srcs[0]);
      case ir_unop_unpack_sampler_2x32:
   case ir_unop_unpack_image_2x32:
   case ir_unop_unpack_double_2x32:
   case ir_unop_unpack_int_2x32:
   case ir_unop_unpack_uint_2x32:
      result = nir_unpack_64_2x32(&b, srcs[0]);
      case ir_unop_bitfield_reverse:
      result = nir_bitfield_reverse(&b, srcs[0]);
      case ir_unop_bit_count:
      result = nir_bit_count(&b, srcs[0]);
      case ir_unop_find_msb:
      switch (types[0]) {
   case GLSL_TYPE_UINT:
      result = nir_ufind_msb(&b, srcs[0]);
      case GLSL_TYPE_INT:
      result = nir_ifind_msb(&b, srcs[0]);
      default:
         }
      case ir_unop_find_lsb:
      result = nir_find_lsb(&b, srcs[0]);
         case ir_unop_get_buffer_size: {
      nir_intrinsic_instr *load = nir_intrinsic_instr_create(
      this->shader,
      load->num_components = ir->type->vector_elements;
   load->src[0] = nir_src_for_ssa(evaluate_rvalue(ir->operands[0]));
   unsigned bit_size = glsl_get_bit_size(ir->type);
   add_instr(&load->instr, ir->type->vector_elements, bit_size);
               case ir_unop_atan:
      result = nir_atan(&b, srcs[0]);
         case ir_binop_add:
      result = type_is_float(out_type) ? nir_fadd(&b, srcs[0], srcs[1])
            case ir_binop_add_sat:
      result = type_is_signed(out_type) ? nir_iadd_sat(&b, srcs[0], srcs[1])
            case ir_binop_sub:
      result = type_is_float(out_type) ? nir_fsub(&b, srcs[0], srcs[1])
            case ir_binop_sub_sat:
      result = type_is_signed(out_type) ? nir_isub_sat(&b, srcs[0], srcs[1])
            case ir_binop_abs_sub:
      /* out_type is always unsigned for ir_binop_abs_sub, so we have to key
   * on the type of the sources.
   */
   result = type_is_signed(types[0]) ? nir_uabs_isub(&b, srcs[0], srcs[1])
            case ir_binop_avg:
      result = type_is_signed(out_type) ? nir_ihadd(&b, srcs[0], srcs[1])
            case ir_binop_avg_round:
      result = type_is_signed(out_type) ? nir_irhadd(&b, srcs[0], srcs[1])
            case ir_binop_mul_32x16:
      result = type_is_signed(out_type) ? nir_imul_32x16(&b, srcs[0], srcs[1])
            case ir_binop_mul:
      if (type_is_float(out_type))
         else if (out_type == GLSL_TYPE_INT64 &&
            (ir->operands[0]->type->base_type == GLSL_TYPE_INT ||
      else if (out_type == GLSL_TYPE_UINT64 &&
            (ir->operands[0]->type->base_type == GLSL_TYPE_UINT ||
      else
            case ir_binop_div:
      if (type_is_float(out_type))
         else if (type_is_signed(out_type))
         else
            case ir_binop_mod:
      result = type_is_float(out_type) ? nir_fmod(&b, srcs[0], srcs[1])
            case ir_binop_min:
      if (type_is_float(out_type))
         else if (type_is_signed(out_type))
         else
            case ir_binop_max:
      if (type_is_float(out_type))
         else if (type_is_signed(out_type))
         else
            case ir_binop_pow: result = nir_fpow(&b, srcs[0], srcs[1]); break;
   case ir_binop_bit_and: result = nir_iand(&b, srcs[0], srcs[1]); break;
   case ir_binop_bit_or: result = nir_ior(&b, srcs[0], srcs[1]); break;
   case ir_binop_bit_xor: result = nir_ixor(&b, srcs[0], srcs[1]); break;
   case ir_binop_logic_and:
      result = nir_iand(&b, srcs[0], srcs[1]);
      case ir_binop_logic_or:
      result = nir_ior(&b, srcs[0], srcs[1]);
      case ir_binop_logic_xor:
      result = nir_ixor(&b, srcs[0], srcs[1]);
      case ir_binop_lshift: result = nir_ishl(&b, srcs[0], nir_u2u32(&b, srcs[1])); break;
   case ir_binop_rshift:
      result = (type_is_signed(out_type)) ? nir_ishr(&b, srcs[0], nir_u2u32(&b, srcs[1]))
            case ir_binop_imul_high:
      result = (out_type == GLSL_TYPE_INT) ? nir_imul_high(&b, srcs[0], srcs[1])
            case ir_binop_carry:  result = nir_uadd_carry(&b, srcs[0], srcs[1]);  break;
   case ir_binop_borrow: result = nir_usub_borrow(&b, srcs[0], srcs[1]); break;
   case ir_binop_less:
      if (type_is_float(types[0]))
         else if (type_is_signed(types[0]))
         else
            case ir_binop_gequal:
      if (type_is_float(types[0]))
         else if (type_is_signed(types[0]))
         else
            case ir_binop_equal:
      if (type_is_float(types[0]))
         else
            case ir_binop_nequal:
      if (type_is_float(types[0]))
         else
            case ir_binop_all_equal:
      if (type_is_float(types[0])) {
      switch (ir->operands[0]->type->vector_elements) {
      case 1: result = nir_feq(&b, srcs[0], srcs[1]); break;
   case 2: result = nir_ball_fequal2(&b, srcs[0], srcs[1]); break;
   case 3: result = nir_ball_fequal3(&b, srcs[0], srcs[1]); break;
   case 4: result = nir_ball_fequal4(&b, srcs[0], srcs[1]); break;
   default:
         } else {
      switch (ir->operands[0]->type->vector_elements) {
      case 1: result = nir_ieq(&b, srcs[0], srcs[1]); break;
   case 2: result = nir_ball_iequal2(&b, srcs[0], srcs[1]); break;
   case 3: result = nir_ball_iequal3(&b, srcs[0], srcs[1]); break;
   case 4: result = nir_ball_iequal4(&b, srcs[0], srcs[1]); break;
   default:
         }
      case ir_binop_any_nequal:
      if (type_is_float(types[0])) {
      switch (ir->operands[0]->type->vector_elements) {
      case 1: result = nir_fneu(&b, srcs[0], srcs[1]); break;
   case 2: result = nir_bany_fnequal2(&b, srcs[0], srcs[1]); break;
   case 3: result = nir_bany_fnequal3(&b, srcs[0], srcs[1]); break;
   case 4: result = nir_bany_fnequal4(&b, srcs[0], srcs[1]); break;
   default:
         } else {
      switch (ir->operands[0]->type->vector_elements) {
      case 1: result = nir_ine(&b, srcs[0], srcs[1]); break;
   case 2: result = nir_bany_inequal2(&b, srcs[0], srcs[1]); break;
   case 3: result = nir_bany_inequal3(&b, srcs[0], srcs[1]); break;
   case 4: result = nir_bany_inequal4(&b, srcs[0], srcs[1]); break;
   default:
         }
      case ir_binop_dot:
      result = nir_fdot(&b, srcs[0], srcs[1]);
         case ir_binop_vector_extract:
      result = nir_vector_extract(&b, srcs[0], srcs[1]);
      case ir_triop_vector_insert:
      result = nir_vector_insert(&b, srcs[0], srcs[1], srcs[2]);
         case ir_binop_atan2:
      result = nir_atan2(&b, srcs[0], srcs[1]);
         case ir_binop_ldexp: result = nir_ldexp(&b, srcs[0], srcs[1]); break;
   case ir_triop_fma:
      result = nir_ffma(&b, srcs[0], srcs[1], srcs[2]);
      case ir_triop_lrp:
      result = nir_flrp(&b, srcs[0], srcs[1], srcs[2]);
      case ir_triop_csel:
      result = nir_bcsel(&b, srcs[0], srcs[1], srcs[2]);
      case ir_triop_bitfield_extract:
      result = ir->type->is_int_16_32() ?
                  if (ir->type->base_type == GLSL_TYPE_INT16) {
         } else if (ir->type->base_type == GLSL_TYPE_UINT16) {
                     case ir_quadop_bitfield_insert:
      result = nir_bitfield_insert(&b,
                  if (ir->type->base_type == GLSL_TYPE_INT16) {
         } else if (ir->type->base_type == GLSL_TYPE_UINT16) {
                     case ir_quadop_vector:
      result = nir_vec(&b, srcs, ir->type->vector_elements);
         default:
                  /* The bit-size of the NIR SSA value must match the bit-size of the
   * original GLSL IR expression.
   */
      }
      void
   nir_visitor::visit(ir_swizzle *ir)
   {
      unsigned swizzle[4] = { ir->mask.x, ir->mask.y, ir->mask.z, ir->mask.w };
   result = nir_swizzle(&b, evaluate_rvalue(ir->val), swizzle,
      }
      void
   nir_visitor::visit(ir_texture *ir)
   {
      unsigned num_srcs;
   nir_texop op;
   switch (ir->op) {
   case ir_tex:
      op = nir_texop_tex;
   num_srcs = 1; /* coordinate */
         case ir_txb:
   case ir_txl:
      op = (ir->op == ir_txb) ? nir_texop_txb : nir_texop_txl;
   num_srcs = 2; /* coordinate, bias/lod */
         case ir_txd:
      op = nir_texop_txd; /* coordinate, dPdx, dPdy */
   num_srcs = 3;
         case ir_txf:
      op = nir_texop_txf;
   if (ir->lod_info.lod != NULL)
         else
               case ir_txf_ms:
      op = nir_texop_txf_ms;
   num_srcs = 2; /* coordinate, sample_index */
         case ir_txs:
      op = nir_texop_txs;
   if (ir->lod_info.lod != NULL)
         else
               case ir_lod:
      op = nir_texop_lod;
   num_srcs = 1; /* coordinate */
         case ir_tg4:
      op = nir_texop_tg4;
   num_srcs = 1; /* coordinate */
         case ir_query_levels:
      op = nir_texop_query_levels;
   num_srcs = 0;
         case ir_texture_samples:
      op = nir_texop_texture_samples;
   num_srcs = 0;
         case ir_samples_identical:
      op = nir_texop_samples_identical;
   num_srcs = 1; /* coordinate */
         default:
                  if (ir->projector != NULL)
         if (ir->shadow_comparator != NULL)
         /* offsets are constants we store inside nir_tex_intrs.offsets */
   if (ir->offset != NULL && !ir->offset->type->is_array())
         if (ir->clamp != NULL)
            /* Add one for the texture deref */
                     instr->op = op;
   instr->sampler_dim =
         instr->is_array = ir->sampler->type->sampler_array;
            const glsl_type *dest_type
         assert(dest_type != glsl_type::error_type);
   if (instr->is_shadow)
         instr->dest_type = nir_get_nir_type_for_glsl_type(dest_type);
                     /* check for bindless handles */
   if (!nir_deref_mode_is(sampler_deref, nir_var_uniform) ||
      nir_deref_instr_get_variable(sampler_deref)->data.bindless) {
   nir_def *load = nir_load_deref(&b, sampler_deref);
   instr->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_handle, load);
      } else {
      instr->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         instr->src[1] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
                        if (ir->coordinate != NULL) {
      instr->coord_components = ir->coordinate->type->vector_elements;
   instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_coord,
                     if (ir->projector != NULL) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_projector,
                     if (ir->shadow_comparator != NULL) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_comparator,
                     if (ir->offset != NULL) {
      if (ir->offset->type->is_array()) {
      for (int i = 0; i < ir->offset->type->array_size(); i++) {
                     for (unsigned j = 0; j < 2; ++j) {
      int val = c->get_int_component(j);
            } else {
               instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_offset,
                        if (ir->clamp) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_min_lod,
                     switch (ir->op) {
   case ir_txb:
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_bias,
         src_number++;
         case ir_txl:
   case ir_txf:
   case ir_txs:
      if (ir->lod_info.lod != NULL) {
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_lod,
            }
         case ir_txd:
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_ddx,
         src_number++;
   instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_ddy,
         src_number++;
         case ir_txf_ms:
      instr->src[src_number] = nir_tex_src_for_ssa(nir_tex_src_ms_index,
         src_number++;
         case ir_tg4:
      instr->component = ir->lod_info.component->as_constant()->value.u[0];
         default:
                           unsigned bit_size = glsl_get_bit_size(dest_type);
      }
      void
   nir_visitor::visit(ir_constant *ir)
   {
      /*
   * We don't know if this variable is an array or struct that gets
   * dereferenced, so do the safe thing an make it a variable with a
   * constant initializer and return a dereference.
            nir_variable *var =
         var->data.read_only = true;
               }
      void
   nir_visitor::visit(ir_dereference_variable *ir)
   {
      if (ir->variable_referenced()->data.mode == ir_var_function_out) {
               foreach_in_list(ir_variable, param, &sig->parameters) {
      if (param == ir->variable_referenced()) {
         }
               this->deref = nir_build_deref_cast(&b, nir_load_param(&b, i),
                              struct hash_entry *entry =
         assert(entry);
               }
      void
   nir_visitor::visit(ir_dereference_record *ir)
   {
               int field_index = ir->field_idx;
            /* sparse texture variable is a struct for ir_variable, but it has been
   * converted to a vector for nir_variable.
   */
   if (this->deref->deref_type == nir_deref_type_var &&
      _mesa_set_search(this->sparse_variable_set, this->deref->var)) {
   nir_def *load = nir_load_deref(&b, this->deref);
            nir_def *ssa;
   const glsl_type *type = ir->record->type;
   if (field_index == type->field_index("code")) {
      /* last channel holds residency code */
      } else {
               unsigned mask = BITFIELD_MASK(load->num_components - 1);
               /* still need to create a deref for return */
   nir_variable *tmp =
         this->deref = nir_build_deref_var(&b, tmp);
      } else
      }
      void
   nir_visitor::visit(ir_dereference_array *ir)
   {
                           }
      void
   nir_visitor::visit(ir_barrier *)
   {
      if (shader->info.stage == MESA_SHADER_COMPUTE) {
      nir_barrier(&b, SCOPE_WORKGROUP, SCOPE_WORKGROUP,
      } else if (shader->info.stage == MESA_SHADER_TESS_CTRL) {
      nir_barrier(&b, SCOPE_WORKGROUP, SCOPE_WORKGROUP,
         }
      nir_shader *
   glsl_float64_funcs_to_nir(struct gl_context *ctx,
         {
      /* We pretend it's a vertex shader.  Ultimately, the stage shouldn't
   * matter because we're not optimizing anything here.
   */
   struct gl_shader *sh = _mesa_new_shader(-1, MESA_SHADER_VERTEX);
   sh->Source = float64_source;
   sh->CompileStatus = COMPILE_FAILURE;
            if (!sh->CompileStatus) {
      if (sh->InfoLog) {
      _mesa_problem(ctx,
            }
                        nir_visitor v1(&ctx->Const, nir);
   nir_function_visitor v2(&v1);
   v2.run(sh->ir);
            /* _mesa_delete_shader will try to free sh->Source but it's static const */
   sh->Source = NULL;
                     NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, nir_inline_functions);
            /* Do some optimizations to clean up the shader now.  By optimizing the
   * functions in the library, we avoid having to re-do that work every
   * time we inline a copy of a function.  Reducing basic blocks also helps
   * with compile times.
   */
   NIR_PASS_V(nir, nir_lower_vars_to_ssa);
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_function_temp, NULL);
   NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_dce);
   NIR_PASS_V(nir, nir_opt_cse);
   NIR_PASS_V(nir, nir_opt_gcm, true);
   NIR_PASS_V(nir, nir_opt_peephole_select, 1, false, false);
               }
