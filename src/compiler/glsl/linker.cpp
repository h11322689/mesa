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
   * \file linker.cpp
   * GLSL linker implementation
   *
   * Given a set of shaders that are to be linked to generate a final program,
   * there are three distinct stages.
   *
   * In the first stage shaders are partitioned into groups based on the shader
   * type.  All shaders of a particular type (e.g., vertex shaders) are linked
   * together.
   *
   *   - Undefined references in each shader are resolve to definitions in
   *     another shader.
   *   - Types and qualifiers of uniforms, outputs, and global variables defined
   *     in multiple shaders with the same name are verified to be the same.
   *   - Initializers for uniforms and global variables defined
   *     in multiple shaders with the same name are verified to be the same.
   *
   * The result, in the terminology of the GLSL spec, is a set of shader
   * executables for each processing unit.
   *
   * After the first stage is complete, a series of semantic checks are performed
   * on each of the shader executables.
   *
   *   - Each shader executable must define a \c main function.
   *   - Each vertex shader executable must write to \c gl_Position.
   *   - Each fragment shader executable must write to either \c gl_FragData or
   *     \c gl_FragColor.
   *
   * In the final stage individual shader executables are linked to create a
   * complete exectuable.
   *
   *   - Types of uniforms defined in multiple shader stages with the same name
   *     are verified to be the same.
   *   - Initializers for uniforms defined in multiple shader stages with the
   *     same name are verified to be the same.
   *   - Types and qualifiers of outputs defined in one stage are verified to
   *     be the same as the types and qualifiers of inputs defined with the same
   *     name in a later stage.
   *
   * \author Ian Romanick <ian.d.romanick@intel.com>
   */
      #include <ctype.h>
   #include "util/strndup.h"
   #include "glsl_symbol_table.h"
   #include "glsl_parser_extras.h"
   #include "ir.h"
   #include "nir.h"
   #include "program.h"
   #include "program/prog_instruction.h"
   #include "program/program.h"
   #include "util/mesa-sha1.h"
   #include "util/set.h"
   #include "string_to_uint_map.h"
   #include "linker.h"
   #include "linker_util.h"
   #include "ir_optimization.h"
   #include "ir_rvalue_visitor.h"
   #include "ir_uniform.h"
   #include "builtin_functions.h"
   #include "shader_cache.h"
   #include "util/u_string.h"
   #include "util/u_math.h"
         #include "main/shaderobj.h"
   #include "main/enums.h"
   #include "main/mtypes.h"
   #include "main/context.h"
         namespace {
      struct find_variable {
      const char *name;
               };
      /**
   * Visitor that determines whether or not a variable is ever written.
   * Note: this is only considering if the variable is statically written
   * (= regardless of the runtime flow of control)
   *
   * Use \ref find_assignments for convenience.
   */
   class find_assignment_visitor : public ir_hierarchical_visitor {
   public:
      find_assignment_visitor(unsigned num_vars,
               {
            virtual ir_visitor_status visit_enter(ir_assignment *ir)
   {
                           virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      foreach_two_lists(formal_node, &ir->callee->parameters,
                           if (sig_param->data.mode == ir_var_function_out ||
      sig_param->data.mode == ir_var_function_inout) {
   ir_variable *var = param_rval->variable_referenced();
   if (var && check_variable_name(var->name) == visit_stop)
                  if (ir->return_deref != NULL) {
               if (check_variable_name(var->name) == visit_stop)
                        private:
      ir_visitor_status check_variable_name(const char *name)
   {
      for (unsigned i = 0; i < num_variables; ++i) {
      if (strcmp(variables[i]->name, name) == 0) {
                        assert(num_found < num_variables);
   if (++num_found == num_variables)
      }
                           private:
      unsigned num_variables;           /**< Number of variables to find */
   unsigned num_found;               /**< Number of variables already found */
      };
      /**
   * Determine whether or not any of NULL-terminated list of variables is ever
   * written to.
   */
   static void
   find_assignments(exec_list *ir, find_variable * const *vars)
   {
               for (find_variable * const *v = vars; *v; ++v)
            find_assignment_visitor visitor(num_variables, vars);
      }
      /**
   * Determine whether or not the given variable is ever written to.
   */
   static void
   find_assignments(exec_list *ir, find_variable *var)
   {
      find_assignment_visitor visitor(1, &var);
      }
      /**
   * Visitor that determines whether or not a variable is ever read.
   */
   class find_deref_visitor : public ir_hierarchical_visitor {
   public:
      find_deref_visitor(const char *name)
         {
                  virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      if (strcmp(this->name, ir->var->name) == 0) {
      this->found = true;
                           bool variable_found() const
   {
               private:
      const char *name;       /**< Find writes to a variable with this name. */
      };
         /**
   * A visitor helper that provides methods for updating the types of
   * ir_dereferences.  Classes that update variable types (say, updating
   * array sizes) will want to use this so that dereference types stay in sync.
   */
   class deref_type_updater : public ir_hierarchical_visitor {
   public:
      virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      ir->type = ir->var->type;
               virtual ir_visitor_status visit_leave(ir_dereference_array *ir)
   {
      const glsl_type *const vt = ir->array->type;
   if (vt->is_array())
                     virtual ir_visitor_status visit_leave(ir_dereference_record *ir)
   {
      ir->type = ir->record->type->fields.structure[ir->field_idx].type;
         };
         class array_resize_visitor : public deref_type_updater {
   public:
               unsigned num_vertices;
   gl_shader_program *prog;
            array_resize_visitor(unsigned num_vertices,
               {
      this->num_vertices = num_vertices;
   this->prog = prog;
               virtual ~array_resize_visitor()
   {
                  virtual ir_visitor_status visit(ir_variable *var)
   {
      if (!var->type->is_array() || var->data.mode != ir_var_shader_in ||
                           if (stage == MESA_SHADER_GEOMETRY) {
      /* Generate a link error if the shader has declared this array with
   * an incorrect size.
   */
   if (!var->data.implicit_sized_array &&
      size && size != this->num_vertices) {
   linker_error(this->prog, "size of array %s declared as %u, "
                           /* Generate a link error if the shader attempts to access an input
   * array using an index too large for its actual size assigned at
   * link time.
   */
   if (var->data.max_array_access >= (int)this->num_vertices) {
      linker_error(this->prog, "%s shader accesses element %i of "
               "%s, but only %i input vertices\n",
                  var->type = glsl_type::get_array_instance(var->type->fields.array,
                        };
      class array_length_to_const_visitor : public ir_rvalue_visitor {
   public:
      array_length_to_const_visitor()
   {
                  virtual ~array_length_to_const_visitor()
   {
                           virtual void handle_rvalue(ir_rvalue **rvalue)
   {
      if (*rvalue == NULL || (*rvalue)->ir_type != ir_type_expression)
            ir_expression *expr = (*rvalue)->as_expression();
   if (expr) {
      if (expr->operation == ir_unop_implicitly_sized_array_length) {
      assert(!expr->operands[0]->type->is_unsized_array());
   ir_constant *constant = new(expr)
         if (constant) {
                     };
      /**
   * Visitor that determines the highest stream id to which a (geometry) shader
   * emits vertices. It also checks whether End{Stream}Primitive is ever called.
   */
   class find_emit_vertex_visitor : public ir_hierarchical_visitor {
   public:
      find_emit_vertex_visitor(int max_allowed)
      : max_stream_allowed(max_allowed),
   invalid_stream_id(0),
   invalid_stream_id_from_emit_vertex(false),
   end_primitive_found(false),
      {
                  virtual ir_visitor_status visit_leave(ir_emit_vertex *ir)
   {
               if (stream_id < 0) {
      invalid_stream_id = stream_id;
   invalid_stream_id_from_emit_vertex = true;
               if (stream_id > max_stream_allowed) {
      invalid_stream_id = stream_id;
   invalid_stream_id_from_emit_vertex = true;
                                    virtual ir_visitor_status visit_leave(ir_end_primitive *ir)
   {
                        if (stream_id < 0) {
      invalid_stream_id = stream_id;
   invalid_stream_id_from_emit_vertex = false;
               if (stream_id > max_stream_allowed) {
      invalid_stream_id = stream_id;
   invalid_stream_id_from_emit_vertex = false;
                                    bool error()
   {
                  const char *error_func()
   {
      return invalid_stream_id_from_emit_vertex ?
               int error_stream()
   {
                  unsigned active_stream_mask()
   {
                  bool uses_end_primitive()
   {
               private:
      int max_stream_allowed;
   int invalid_stream_id;
   bool invalid_stream_id_from_emit_vertex;
   bool end_primitive_found;
      };
      } /* anonymous namespace */
      void
   linker_error(gl_shader_program *prog, const char *fmt, ...)
   {
               ralloc_strcat(&prog->data->InfoLog, "error: ");
   va_start(ap, fmt);
   ralloc_vasprintf_append(&prog->data->InfoLog, fmt, ap);
               }
         void
   linker_warning(gl_shader_program *prog, const char *fmt, ...)
   {
               ralloc_strcat(&prog->data->InfoLog, "warning: ");
   va_start(ap, fmt);
   ralloc_vasprintf_append(&prog->data->InfoLog, fmt, ap);
         }
         /**
   * Set clip_distance_array_size based and cull_distance_array_size on the given
   * shader.
   *
   * Also check for errors based on incorrect usage of gl_ClipVertex and
   * gl_ClipDistance and gl_CullDistance.
   * Additionally test whether the arrays gl_ClipDistance and gl_CullDistance
   * exceed the maximum size defined by gl_MaxCombinedClipAndCullDistances.
   *
   * Return false if an error was reported.
   */
   static void
   analyze_clip_cull_usage(struct gl_shader_program *prog,
                     {
      if (consts->DoDCEBeforeClipCullAnalysis) {
      /* Remove dead functions to avoid raising an error (eg: dead function
   * writes to gl_ClipVertex, and main() writes to gl_ClipDistance).
   */
               info->clip_distance_array_size = 0;
            if (prog->GLSL_Version >= (prog->IsES ? 300 : 130)) {
      /* From section 7.1 (Vertex Shader Special Variables) of the
   * GLSL 1.30 spec:
   *
   *   "It is an error for a shader to statically write both
   *   gl_ClipVertex and gl_ClipDistance."
   *
   * This does not apply to GLSL ES shaders, since GLSL ES defines neither
   * gl_ClipVertex nor gl_ClipDistance. However with
   * GL_EXT_clip_cull_distance, this functionality is exposed in ES 3.0.
   */
   find_variable gl_ClipDistance("gl_ClipDistance");
   find_variable gl_CullDistance("gl_CullDistance");
   find_variable gl_ClipVertex("gl_ClipVertex");
   find_variable * const variables[] = {
      &gl_ClipDistance,
   &gl_CullDistance,
   !prog->IsES ? &gl_ClipVertex : NULL,
      };
            /* From the ARB_cull_distance spec:
   *
   * It is a compile-time or link-time error for the set of shaders forming
   * a program to statically read or write both gl_ClipVertex and either
   * gl_ClipDistance or gl_CullDistance.
   *
   * This does not apply to GLSL ES shaders, since GLSL ES doesn't define
   * gl_ClipVertex.
   */
   if (!prog->IsES) {
      if (gl_ClipVertex.found && gl_ClipDistance.found) {
      linker_error(prog, "%s shader writes to both `gl_ClipVertex' "
                  }
   if (gl_ClipVertex.found && gl_CullDistance.found) {
      linker_error(prog, "%s shader writes to both `gl_ClipVertex' "
                              if (gl_ClipDistance.found) {
      ir_variable *clip_distance_var =
         assert(clip_distance_var);
      }
   if (gl_CullDistance.found) {
      ir_variable *cull_distance_var =
         assert(cull_distance_var);
      }
   /* From the ARB_cull_distance spec:
   *
   * It is a compile-time or link-time error for the set of shaders forming
   * a program to have the sum of the sizes of the gl_ClipDistance and
   * gl_CullDistance arrays to be larger than
   * gl_MaxCombinedClipAndCullDistances.
   */
   if ((uint32_t)(info->clip_distance_array_size + info->cull_distance_array_size) >
      consts->MaxClipPlanes) {
   linker_error(prog, "%s shader: the combined size of "
               "'gl_ClipDistance' and 'gl_CullDistance' size cannot "
   "be larger than "
            }
         /**
   * Verify that a vertex shader executable meets all semantic requirements.
   *
   * Also sets info.clip_distance_array_size and
   * info.cull_distance_array_size as a side effect.
   *
   * \param shader  Vertex shader executable to be verified
   */
   static void
   validate_vertex_shader_executable(struct gl_shader_program *prog,
               {
      if (shader == NULL)
            /* From the GLSL 1.10 spec, page 48:
   *
   *     "The variable gl_Position is available only in the vertex
   *      language and is intended for writing the homogeneous vertex
   *      position. All executions of a well-formed vertex shader
   *      executable must write a value into this variable. [...] The
   *      variable gl_Position is available only in the vertex
   *      language and is intended for writing the homogeneous vertex
   *      position. All executions of a well-formed vertex shader
   *      executable must write a value into this variable."
   *
   * while in GLSL 1.40 this text is changed to:
   *
   *     "The variable gl_Position is available only in the vertex
   *      language and is intended for writing the homogeneous vertex
   *      position. It can be written at any time during shader
   *      execution. It may also be read back by a vertex shader
   *      after being written. This value will be used by primitive
   *      assembly, clipping, culling, and other fixed functionality
   *      operations, if present, that operate on primitives after
   *      vertex processing has occurred. Its value is undefined if
   *      the vertex shader executable does not write gl_Position."
   *
   * All GLSL ES Versions are similar to GLSL 1.40--failing to write to
   * gl_Position is not an error.
   */
   if (prog->GLSL_Version < (prog->IsES ? 300 : 140)) {
      find_variable gl_Position("gl_Position");
   find_assignments(shader->ir, &gl_Position);
   if (!gl_Position.found) {
   if (prog->IsES) {
      linker_warning(prog,
            } else {
      linker_error(prog,
      }
                        }
      static void
   validate_tess_eval_shader_executable(struct gl_shader_program *prog,
               {
      if (shader == NULL)
               }
         /**
   * Verify that a fragment shader executable meets all semantic requirements
   *
   * \param shader  Fragment shader executable to be verified
   */
   static void
   validate_fragment_shader_executable(struct gl_shader_program *prog,
         {
      if (shader == NULL)
            find_variable gl_FragColor("gl_FragColor");
   find_variable gl_FragData("gl_FragData");
   find_variable * const variables[] = { &gl_FragColor, &gl_FragData, NULL };
            if (gl_FragColor.found && gl_FragData.found) {
      linker_error(prog,  "fragment shader writes to both "
         }
      /**
   * Verify that a geometry shader executable meets all semantic requirements
   *
   * Also sets prog->Geom.VerticesIn, and info.clip_distance_array_sizeand
   * info.cull_distance_array_size as a side effect.
   *
   * \param shader Geometry shader executable to be verified
   */
   static void
   validate_geometry_shader_executable(struct gl_shader_program *prog,
               {
      if (shader == NULL)
            unsigned num_vertices =
                     }
      /**
   * Check if geometry shaders emit to non-zero streams and do corresponding
   * validations.
   */
   static void
   validate_geometry_shader_emissions(const struct gl_constants *consts,
         {
               if (sh != NULL) {
      find_emit_vertex_visitor emit_vertex(consts->MaxVertexStreams - 1);
   emit_vertex.run(sh->ir);
   if (emit_vertex.error()) {
      linker_error(prog, "Invalid call %s(%d). Accepted values for the "
               "stream parameter are in the range [0, %d].\n",
      }
   prog->Geom.ActiveStreamMask = emit_vertex.active_stream_mask();
            /* From the ARB_gpu_shader5 spec:
   *
   *   "Multiple vertex streams are supported only if the output primitive
   *    type is declared to be "points".  A program will fail to link if it
   *    contains a geometry shader calling EmitStreamVertex() or
   *    EndStreamPrimitive() if its output primitive type is not "points".
   *
   * However, in the same spec:
   *
   *   "The function EmitVertex() is equivalent to calling EmitStreamVertex()
   *    with <stream> set to zero."
   *
   * And:
   *
   *   "The function EndPrimitive() is equivalent to calling
   *    EndStreamPrimitive() with <stream> set to zero."
   *
   * Since we can call EmitVertex() and EndPrimitive() when we output
   * primitives other than points, calling EmitStreamVertex(0) or
   * EmitEndPrimitive(0) should not produce errors. This it also what Nvidia
   * does. We can use prog->Geom.ActiveStreamMask to check whether only the
   * first (zero) stream is active.
   * stream.
   */
   if (prog->Geom.ActiveStreamMask & ~(1 << 0) &&
      sh->Program->info.gs.output_primitive != GL_POINTS) {
   linker_error(prog, "EmitStreamVertex(n) and EndStreamPrimitive(n) "
            }
      bool
   validate_intrastage_arrays(struct gl_shader_program *prog,
                     {
      /* Consider the types to be "the same" if both types are arrays
   * of the same type and one of the arrays is implicitly sized.
   * In addition, set the type of the linked variable to the
   * explicitly sized array.
   */
   if (var->type->is_array() && existing->type->is_array()) {
      const glsl_type *no_array_var = var->type->fields.array;
   const glsl_type *no_array_existing = existing->type->fields.array;
            type_matches = (match_precision ?
                  if (type_matches &&
      ((var->type->length == 0)|| (existing->type->length == 0))) {
   if (var->type->length != 0) {
      if ((int)var->type->length <= existing->data.max_array_access) {
      linker_error(prog, "%s `%s' declared as type "
               "`%s' but outermost dimension has an index"
   " of `%i'\n",
      }
   existing->type = var->type;
      } else if (existing->type->length != 0) {
      if((int)existing->type->length <= var->data.max_array_access &&
      !existing->data.from_ssbo_unsized_array) {
   linker_error(prog, "%s `%s' declared as type "
               "`%s' but outermost dimension has an index"
   " of `%i'\n",
      }
            }
      }
         /**
   * Perform validation of global variables used across multiple shaders
   */
   static void
   cross_validate_globals(const struct gl_constants *consts,
                     {
      foreach_in_list(ir_instruction, node, ir) {
               if (var == NULL)
            if (uniforms_only && (var->data.mode != ir_var_uniform && var->data.mode != ir_var_shader_storage))
            /* don't cross validate subroutine uniforms */
   if (var->type->contains_subroutine())
            /* Don't cross validate interface instances. These are only relevant
   * inside a shader. The cross validation is done at the Interface Block
   * name level.
   */
   if (var->is_interface_instance())
            /* Don't cross validate temporaries that are at global scope.  These
   * will eventually get pulled into the shaders 'main'.
   */
   if (var->data.mode == ir_var_temporary)
            /* If a global with this name has already been seen, verify that the
   * new instance has the same type.  In addition, if the globals have
   * initializers, the values of the initializers must be the same.
   */
   ir_variable *const existing = variables->get_variable(var->name);
   if (existing != NULL) {
      /* Check if types match. */
   if (var->type != existing->type) {
      if (!validate_intrastage_arrays(prog, var, existing)) {
      /* If it is an unsized array in a Shader Storage Block,
   * two different shaders can access to different elements.
   * Because of that, they might be converted to different
   * sized arrays, then check that they are compatible but
   * ignore the array size.
   */
   if (!(var->data.mode == ir_var_shader_storage &&
         var->data.from_ssbo_unsized_array &&
   existing->data.mode == ir_var_shader_storage &&
   existing->data.from_ssbo_unsized_array &&
      linker_error(prog, "%s `%s' declared as type "
                  "`%s' and type `%s'\n",
                        if (var->data.explicit_location) {
      if (existing->data.explicit_location
      && (var->data.location != existing->data.location)) {
   linker_error(prog, "explicit locations for %s "
                           if (var->data.location_frac != existing->data.location_frac) {
      linker_error(prog, "explicit components for %s `%s' have "
                     existing->data.location = var->data.location;
      } else {
      /* Check if uniform with implicit location was marked explicit
   * by earlier shader stage. If so, mark it explicit in this stage
   * too to make sure later processing does not treat it as
   * implicit one.
   */
   if (existing->data.explicit_location) {
      var->data.location = existing->data.location;
                  /* From the GLSL 4.20 specification:
   * "A link error will result if two compilation units in a program
   *  specify different integer-constant bindings for the same
   *  opaque-uniform name.  However, it is not an error to specify a
   *  binding on some but not all declarations for the same name"
   */
   if (var->data.explicit_binding) {
      if (existing->data.explicit_binding &&
      var->data.binding != existing->data.binding) {
   linker_error(prog, "explicit bindings for %s "
                           existing->data.binding = var->data.binding;
               if (var->type->contains_atomic() &&
      var->data.offset != existing->data.offset) {
   linker_error(prog, "offset specifications for %s "
                           /* Validate layout qualifiers for gl_FragDepth.
   *
   * From the AMD/ARB_conservative_depth specs:
   *
   *    "If gl_FragDepth is redeclared in any fragment shader in a
   *    program, it must be redeclared in all fragment shaders in
   *    that program that have static assignments to
   *    gl_FragDepth. All redeclarations of gl_FragDepth in all
   *    fragment shaders in a single program must have the same set
   *    of qualifiers."
   */
   if (strcmp(var->name, "gl_FragDepth") == 0) {
      bool layout_declared = var->data.depth_layout != ir_depth_layout_none;
                  if (layout_declared && layout_differs) {
      linker_error(prog,
                           if (var->data.used && layout_differs) {
      linker_error(prog,
               "If gl_FragDepth is redeclared with a layout "
   "qualifier in any fragment shader, it must be "
                  /* Page 35 (page 41 of the PDF) of the GLSL 4.20 spec says:
   *
   *     "If a shared global has multiple initializers, the
   *     initializers must all be constant expressions, and they
   *     must all have the same value. Otherwise, a link error will
   *     result. (A shared global having only one initializer does
   *     not require that initializer to be a constant expression.)"
   *
   * Previous to 4.20 the GLSL spec simply said that initializers
   * must have the same value.  In this case of non-constant
   * initializers, this was impossible to determine.  As a result,
   * no vendor actually implemented that behavior.  The 4.20
   * behavior matches the implemented behavior of at least one other
   * vendor, so we'll implement that for all GLSL versions.
   * If (at least) one of these constant expressions is implicit,
   * because it was added by glsl_zero_init, we skip the verification.
   */
   if (var->constant_initializer != NULL) {
      if (existing->constant_initializer != NULL &&
      !existing->data.is_implicit_initializer &&
   !var->data.is_implicit_initializer) {
   if (!var->constant_initializer->has_value(existing->constant_initializer)) {
      linker_error(prog, "initializers for %s "
                     } else {
      /* If the first-seen instance of a particular uniform did
   * not have an initializer but a later instance does,
   * replace the former with the later.
   */
   if (!var->data.is_implicit_initializer)
                  if (var->data.has_initializer) {
      if (existing->data.has_initializer
      && (var->constant_initializer == NULL
         linker_error(prog,
               "shared global variable `%s' has multiple "
                  if (existing->data.explicit_invariant != var->data.explicit_invariant) {
      linker_error(prog, "declarations for %s `%s' have "
                  }
   if (existing->data.centroid != var->data.centroid) {
      linker_error(prog, "declarations for %s `%s' have "
                  }
   if (existing->data.sample != var->data.sample) {
      linker_error(prog, "declarations for %s `%s` have "
                  }
   if (existing->data.image_format != var->data.image_format) {
      linker_error(prog, "declarations for %s `%s` have "
                           /* Check the precision qualifier matches for uniform variables on
   * GLSL ES.
   */
   if (!consts->AllowGLSLRelaxedES &&
      prog->IsES && !var->get_interface_type() &&
   existing->data.precision != var->data.precision) {
   if ((existing->data.used && var->data.used) ||
      prog->GLSL_Version >= 300) {
   linker_error(prog, "declarations for %s `%s` have "
                  } else {
      linker_warning(prog, "declarations for %s `%s` have "
                        /* In OpenGL GLSL 3.20 spec, section 4.3.9:
   *
   *   "It is a link-time error if any particular shader interface
   *    contains:
   *
   *    - two different blocks, each having no instance name, and each
   *      having a member of the same name, or
   *
   *    - a variable outside a block, and a block with no instance name,
   *      where the variable has the same name as a member in the block."
   */
   const glsl_type *var_itype = var->get_interface_type();
   const glsl_type *existing_itype = existing->get_interface_type();
   if (var_itype != existing_itype) {
      if (!var_itype || !existing_itype) {
      linker_error(prog, "declarations for %s `%s` are inside block "
               "`%s` and outside a block",
      } else if (strcmp(glsl_get_type_name(var_itype), glsl_get_type_name(existing_itype)) != 0) {
      linker_error(prog, "declarations for %s `%s` are inside blocks "
               "`%s` and `%s`",
   mode_string(var), var->name,
            } else
         }
         /**
   * Perform validation of uniforms used across multiple shader stages
   */
   static void
   cross_validate_uniforms(const struct gl_constants *consts,
         {
      glsl_symbol_table variables;
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
            cross_validate_globals(consts, prog, prog->_LinkedShaders[i]->ir,
         }
      /**
   * Accumulates the array of buffer blocks and checks that all definitions of
   * blocks agree on their contents.
   */
   static bool
   interstage_cross_validate_uniform_blocks(struct gl_shader_program *prog,
         {
      int *ifc_blk_stage_idx[MESA_SHADER_STAGES];
   struct gl_uniform_block *blks = NULL;
   unsigned *num_blks = validate_ssbo ? &prog->data->NumShaderStorageBlocks :
            unsigned max_num_buffer_blocks = 0;
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i]) {
      if (validate_ssbo) {
      max_num_buffer_blocks +=
      } else {
      max_num_buffer_blocks +=
                     for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
               ifc_blk_stage_idx[i] =
         for (unsigned int j = 0; j < max_num_buffer_blocks; j++)
            if (sh == NULL)
            unsigned sh_num_blocks;
   struct gl_uniform_block **sh_blks;
   if (validate_ssbo) {
      sh_num_blocks = prog->_LinkedShaders[i]->Program->info.num_ssbos;
      } else {
      sh_num_blocks = prog->_LinkedShaders[i]->Program->info.num_ubos;
               for (unsigned int j = 0; j < sh_num_blocks; j++) {
                     if (index == -1) {
                     for (unsigned k = 0; k <= i; k++) {
                  /* Reset the block count. This will help avoid various segfaults
   * from api calls that assume the array exists due to the count
   * being non-zero.
   */
   *num_blks = 0;
                              /* Update per stage block pointers to point to the program list.
   * FIXME: We should be able to free the per stage blocks here.
   */
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      for (unsigned j = 0; j < *num_blks; j++) {
                                 struct gl_uniform_block **sh_blks = validate_ssbo ?
                  blks[j].stageref |= sh_blks[stage_index]->stageref;
                     for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
                  if (validate_ssbo)
         else
               }
      /**
   * Verifies the invariance of built-in special variables.
   */
   static bool
   validate_invariant_builtins(struct gl_shader_program *prog,
               {
      const ir_variable *var_vert;
            if (!vert || !frag)
            /*
   * From OpenGL ES Shading Language 1.0 specification
   * (4.6.4 Invariance and Linkage):
   *     "The invariance of varyings that are declared in both the vertex and
   *     fragment shaders must match. For the built-in special variables,
   *     gl_FragCoord can only be declared invariant if and only if
   *     gl_Position is declared invariant. Similarly gl_PointCoord can only
   *     be declared invariant if and only if gl_PointSize is declared
   *     invariant. It is an error to declare gl_FrontFacing as invariant.
   *     The invariance of gl_FrontFacing is the same as the invariance of
   *     gl_Position."
   */
   var_frag = frag->symbols->get_variable("gl_FragCoord");
   if (var_frag && var_frag->data.invariant) {
      var_vert = vert->symbols->get_variable("gl_Position");
   if (var_vert && !var_vert->data.invariant) {
      linker_error(prog,
         "fragment shader built-in `%s' has invariant qualifier, "
   "but vertex shader built-in `%s' lacks invariant qualifier\n",
                  var_frag = frag->symbols->get_variable("gl_PointCoord");
   if (var_frag && var_frag->data.invariant) {
      var_vert = vert->symbols->get_variable("gl_PointSize");
   if (var_vert && !var_vert->data.invariant) {
      linker_error(prog,
         "fragment shader built-in `%s' has invariant qualifier, "
   "but vertex shader built-in `%s' lacks invariant qualifier\n",
                  var_frag = frag->symbols->get_variable("gl_FrontFacing");
   if (var_frag && var_frag->data.invariant) {
      linker_error(prog,
         "fragment shader built-in `%s' can not be declared as invariant\n",
                  }
      /**
   * Populates a shaders symbol table with all global declarations
   */
   static void
   populate_symbol_table(gl_linked_shader *sh, glsl_symbol_table *symbols)
   {
                  }
         /**
   * Remap variables referenced in an instruction tree
   *
   * This is used when instruction trees are cloned from one shader and placed in
   * another.  These trees will contain references to \c ir_variable nodes that
   * do not exist in the target shader.  This function finds these \c ir_variable
   * references and replaces the references with matching variables in the target
   * shader.
   *
   * If there is no matching variable in the target shader, a clone of the
   * \c ir_variable is made and added to the target shader.  The new variable is
   * added to \b both the instruction stream and the symbol table.
   *
   * \param inst         IR tree that is to be processed.
   * \param symbols      Symbol table containing global scope symbols in the
   *                     linked shader.
   * \param instructions Instruction stream where new variable declarations
   *                     should be added.
   */
   static void
   remap_variables(ir_instruction *inst, struct gl_linked_shader *target,
         {
      class remap_visitor : public ir_hierarchical_visitor {
   public:
            {
      this->target = target;
   this->symbols = target->symbols;
   this->instructions = target->ir;
               virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      if (ir->var->data.mode == ir_var_temporary) {
                     assert(var != NULL);
   ir->var = var;
               ir_variable *const existing =
         if (existing != NULL)
                           this->symbols->add_variable(copy);
   this->instructions->push_head(copy);
                        private:
      struct gl_linked_shader *target;
   glsl_symbol_table *symbols;
   exec_list *instructions;
                           }
         /**
   * Move non-declarations from one instruction stream to another
   *
   * The intended usage pattern of this function is to pass the pointer to the
   * head sentinel of a list (i.e., a pointer to the list cast to an \c exec_node
   * pointer) for \c last and \c false for \c make_copies on the first
   * call.  Successive calls pass the return value of the previous call for
   * \c last and \c true for \c make_copies.
   *
   * \param instructions Source instruction stream
   * \param last         Instruction after which new instructions should be
   *                     inserted in the target instruction stream
   * \param make_copies  Flag selecting whether instructions in \c instructions
   *                     should be copied (via \c ir_instruction::clone) into the
   *                     target list or moved.
   *
   * \return
   * The new "last" instruction in the target instruction stream.  This pointer
   * is suitable for use as the \c last parameter of a later call to this
   * function.
   */
   static exec_node *
   move_non_declarations(exec_list *instructions, exec_node *last,
         {
               if (make_copies)
            foreach_in_list_safe(ir_instruction, inst, instructions) {
      if (inst->as_function())
            ir_variable *var = inst->as_variable();
   if ((var != NULL) && (var->data.mode != ir_var_temporary))
            assert(inst->as_assignment()
         || inst->as_call()
            if (make_copies) {
               if (var != NULL)
         else
      } else {
                  last->insert_after(inst);
               if (make_copies)
               }
         /**
   * This class is only used in link_intrastage_shaders() below but declaring
   * it inside that function leads to compiler warnings with some versions of
   * gcc.
   */
   class array_sizing_visitor : public deref_type_updater {
   public:
               array_sizing_visitor()
      : mem_ctx(ralloc_context(NULL)),
      {
            ~array_sizing_visitor()
   {
      _mesa_hash_table_destroy(this->unnamed_interfaces, NULL);
               virtual ir_visitor_status visit(ir_variable *var)
   {
      const glsl_type *type_without_array;
   bool implicit_sized_array = var->data.implicit_sized_array;
   fixup_type(&var->type, var->data.max_array_access,
               var->data.implicit_sized_array = implicit_sized_array;
   type_without_array = var->type->without_array();
   if (var->type->is_interface()) {
      if (interface_contains_unsized_arrays(var->type)) {
      const glsl_type *new_type =
      resize_interface_members(var->type,
            var->type = new_type;
         } else if (type_without_array->is_interface()) {
      if (interface_contains_unsized_arrays(type_without_array)) {
      const glsl_type *new_type =
      resize_interface_members(type_without_array,
            var->change_interface_type(new_type);
         } else if (const glsl_type *ifc_type = var->get_interface_type()) {
      /* Store a pointer to the variable in the unnamed_interfaces
   * hashtable.
   */
   hash_entry *entry =
                           if (interface_vars == NULL) {
      interface_vars = rzalloc_array(mem_ctx, ir_variable *,
         _mesa_hash_table_insert(this->unnamed_interfaces, ifc_type,
      }
   unsigned index = ifc_type->field_index(var->name);
   assert(index < ifc_type->length);
   assert(interface_vars[index] == NULL);
      }
               /**
   * For each unnamed interface block that was discovered while running the
   * visitor, adjust the interface type to reflect the newly assigned array
   * sizes, and fix up the ir_variable nodes to point to the new interface
   * type.
   */
   void fixup_unnamed_interface_types()
   {
      hash_table_call_foreach(this->unnamed_interfaces,
            private:
      /**
   * If the type pointed to by \c type represents an unsized array, replace
   * it with a sized array whose size is determined by max_array_access.
   */
   static void fixup_type(const glsl_type **type, unsigned max_array_access,
         {
      if (!from_ssbo_unsized_array && (*type)->is_unsized_array()) {
      *type = glsl_type::get_array_instance((*type)->fields.array,
         *implicit_sized = true;
                  static const glsl_type *
   update_interface_members_array(const glsl_type *type,
         {
      const glsl_type *element_type = type->fields.array;
   if (element_type->is_array()) {
      const glsl_type *new_array_type =
            } else {
      return glsl_type::get_array_instance(new_interface_type,
                  /**
   * Determine whether the given interface type contains unsized arrays (if
   * it doesn't, array_sizing_visitor doesn't need to process it).
   */
   static bool interface_contains_unsized_arrays(const glsl_type *type)
   {
      for (unsigned i = 0; i < type->length; i++) {
      const glsl_type *elem_type = type->fields.structure[i].type;
   if (elem_type->is_unsized_array())
      }
               /**
   * Create a new interface type based on the given type, with unsized arrays
   * replaced by sized arrays whose size is determined by
   * max_ifc_array_access.
   */
   static const glsl_type *
   resize_interface_members(const glsl_type *type,
               {
      unsigned num_fields = type->length;
   glsl_struct_field *fields = new glsl_struct_field[num_fields];
   memcpy(fields, type->fields.structure,
         for (unsigned i = 0; i < num_fields; i++) {
      bool implicit_sized_array = fields[i].implicit_sized_array;
   /* If SSBO last member is unsized array, we don't replace it by a sized
   * array.
   */
   if (is_ssbo && i == (num_fields - 1))
      fixup_type(&fields[i].type, max_ifc_array_access[i],
      else
      fixup_type(&fields[i].type, max_ifc_array_access[i],
         }
   glsl_interface_packing packing =
         bool row_major = (bool) type->interface_row_major;
   const glsl_type *new_ifc_type =
      glsl_type::get_interface_instance(fields, num_fields,
      delete [] fields;
               static void fixup_unnamed_interface_type(const void *key, void *data,
         {
      const glsl_type *ifc_type = (const glsl_type *) key;
   ir_variable **interface_vars = (ir_variable **) data;
   unsigned num_fields = ifc_type->length;
   glsl_struct_field *fields = new glsl_struct_field[num_fields];
   memcpy(fields, ifc_type->fields.structure,
         bool interface_type_changed = false;
   for (unsigned i = 0; i < num_fields; i++) {
      if (interface_vars[i] != NULL &&
      fields[i].type != interface_vars[i]->type) {
   fields[i].type = interface_vars[i]->type;
         }
   if (!interface_type_changed) {
      delete [] fields;
      }
   glsl_interface_packing packing =
         bool row_major = (bool) ifc_type->interface_row_major;
   const glsl_type *new_ifc_type =
      glsl_type::get_interface_instance(fields, num_fields, packing,
      delete [] fields;
   for (unsigned i = 0; i < num_fields; i++) {
      if (interface_vars[i] != NULL)
                  /**
   * Memory context used to allocate the data in \c unnamed_interfaces.
   */
            /**
   * Hash table from const glsl_type * to an array of ir_variable *'s
   * pointing to the ir_variables constituting each unnamed interface block.
   */
      };
      static bool
   validate_xfb_buffer_stride(const struct gl_constants *consts, unsigned idx,
         {
      /* We will validate doubles at a later stage */
   if (prog->TransformFeedback.BufferStride[idx] % 4) {
      linker_error(prog, "invalid qualifier xfb_stride=%d must be a "
               "multiple of 4 or if its applied to a type that is "
               if (prog->TransformFeedback.BufferStride[idx] / 4 >
      consts->MaxTransformFeedbackInterleavedComponents) {
   linker_error(prog, "The MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS "
                        }
      /**
   * Check for conflicting xfb_stride default qualifiers and store buffer stride
   * for later use.
   */
   static void
   link_xfb_stride_layout_qualifiers(const struct gl_constants *consts,
                     {
      for (unsigned i = 0; i < MAX_FEEDBACK_BUFFERS; i++) {
                  for (unsigned i = 0; i < num_shaders; i++) {
               for (unsigned j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
      if (shader->TransformFeedbackBufferStride[j]) {
      if (prog->TransformFeedback.BufferStride[j] == 0) {
      prog->TransformFeedback.BufferStride[j] =
         if (!validate_xfb_buffer_stride(consts, j, prog))
      } else if (prog->TransformFeedback.BufferStride[j] !=
            linker_error(prog,
               "intrastage shaders defined with conflicting "
   "xfb_stride for buffer %d (%d and %d)\n", j,
                  }
      /**
   * Check for conflicting bindless/bound sampler/image layout qualifiers at
   * global scope.
   */
   static void
   link_bindless_layout_qualifiers(struct gl_shader_program *prog,
               {
      bool bindless_sampler, bindless_image;
            bindless_sampler = bindless_image = false;
            for (unsigned i = 0; i < num_shaders; i++) {
               if (shader->bindless_sampler)
         if (shader->bindless_image)
         if (shader->bound_sampler)
         if (shader->bound_image)
            if ((bindless_sampler && bound_sampler) ||
      (bindless_image && bound_image)) {
   /* From section 4.4.6 of the ARB_bindless_texture spec:
   *
   *     "If both bindless_sampler and bound_sampler, or bindless_image
   *      and bound_image, are declared at global scope in any
   *      compilation unit, a link- time error will be generated."
   */
   linker_error(prog, "both bindless_sampler and bound_sampler, or "
                  }
      /**
   * Check for conflicting viewport_relative settings across shaders, and sets
   * the value for the linked shader.
   */
   static void
   link_layer_viewport_relative_qualifier(struct gl_shader_program *prog,
                     {
               /* Find first shader with explicit layer declaration */
   for (i = 0; i < num_shaders; i++) {
      if (shader_list[i]->redeclares_gl_layer) {
      gl_prog->info.layer_viewport_relative =
                        /* Now make sure that each subsequent shader's explicit layer declaration
   * matches the first one's.
   */
   for (; i < num_shaders; i++) {
      if (shader_list[i]->redeclares_gl_layer &&
      shader_list[i]->layer_viewport_relative !=
   gl_prog->info.layer_viewport_relative) {
   linker_error(prog, "all gl_Layer redeclarations must have identical "
            }
      /**
   * Performs the cross-validation of tessellation control shader vertices and
   * layout qualifiers for the attached tessellation control shaders,
   * and propagates them to the linked TCS and linked shader program.
   */
   static void
   link_tcs_out_layout_qualifiers(struct gl_shader_program *prog,
                     {
      if (gl_prog->info.stage != MESA_SHADER_TESS_CTRL)
                     /* From the GLSL 4.0 spec (chapter 4.3.8.2):
   *
   *     "All tessellation control shader layout declarations in a program
   *      must specify the same output patch vertex count.  There must be at
   *      least one layout qualifier specifying an output patch vertex count
   *      in any program containing tessellation control shaders; however,
   *      such a declaration is not required in all tessellation control
   *      shaders."
            for (unsigned i = 0; i < num_shaders; i++) {
               if (shader->info.TessCtrl.VerticesOut != 0) {
      if (gl_prog->info.tess.tcs_vertices_out != 0 &&
      gl_prog->info.tess.tcs_vertices_out !=
   (unsigned) shader->info.TessCtrl.VerticesOut) {
   linker_error(prog, "tessellation control shader defined with "
               "conflicting output vertex count (%d and %d)\n",
      }
   gl_prog->info.tess.tcs_vertices_out =
                  /* Just do the intrastage -> interstage propagation right now,
   * since we already know we're in the right type of shader program
   * for doing it.
   */
   if (gl_prog->info.tess.tcs_vertices_out == 0) {
      linker_error(prog, "tessellation control shader didn't declare "
               }
         /**
   * Performs the cross-validation of tessellation evaluation shader
   * primitive type, vertex spacing, ordering and point_mode layout qualifiers
   * for the attached tessellation evaluation shaders, and propagates them
   * to the linked TES and linked shader program.
   */
   static void
   link_tes_in_layout_qualifiers(struct gl_shader_program *prog,
                     {
      if (gl_prog->info.stage != MESA_SHADER_TESS_EVAL)
            int point_mode = -1;
            gl_prog->info.tess._primitive_mode = TESS_PRIMITIVE_UNSPECIFIED;
            /* From the GLSL 4.0 spec (chapter 4.3.8.1):
   *
   *     "At least one tessellation evaluation shader (compilation unit) in
   *      a program must declare a primitive mode in its input layout.
   *      Declaration vertex spacing, ordering, and point mode identifiers is
   *      optional.  It is not required that all tessellation evaluation
   *      shaders in a program declare a primitive mode.  If spacing or
   *      vertex ordering declarations are omitted, the tessellation
   *      primitive generator will use equal spacing or counter-clockwise
   *      vertex ordering, respectively.  If a point mode declaration is
   *      omitted, the tessellation primitive generator will produce lines or
   *      triangles according to the primitive mode."
            for (unsigned i = 0; i < num_shaders; i++) {
               if (shader->info.TessEval._PrimitiveMode != TESS_PRIMITIVE_UNSPECIFIED) {
      if (gl_prog->info.tess._primitive_mode != TESS_PRIMITIVE_UNSPECIFIED &&
      gl_prog->info.tess._primitive_mode !=
   shader->info.TessEval._PrimitiveMode) {
   linker_error(prog, "tessellation evaluation shader defined with "
            }
   gl_prog->info.tess._primitive_mode =
               if (shader->info.TessEval.Spacing != 0) {
      if (gl_prog->info.tess.spacing != 0 && gl_prog->info.tess.spacing !=
      shader->info.TessEval.Spacing) {
   linker_error(prog, "tessellation evaluation shader defined with "
            }
               if (shader->info.TessEval.VertexOrder != 0) {
      if (vertex_order != 0 &&
      vertex_order != shader->info.TessEval.VertexOrder) {
   linker_error(prog, "tessellation evaluation shader defined with "
            }
               if (shader->info.TessEval.PointMode != -1) {
      if (point_mode != -1 &&
      point_mode != shader->info.TessEval.PointMode) {
   linker_error(prog, "tessellation evaluation shader defined with "
            }
                     /* Just do the intrastage -> interstage propagation right now,
   * since we already know we're in the right type of shader program
   * for doing it.
   */
   if (gl_prog->info.tess._primitive_mode == TESS_PRIMITIVE_UNSPECIFIED) {
      linker_error(prog,
                           if (gl_prog->info.tess.spacing == TESS_SPACING_UNSPECIFIED)
            if (vertex_order == 0 || vertex_order == GL_CCW)
         else
               if (point_mode == -1 || point_mode == GL_FALSE)
         else
      }
         /**
   * Performs the cross-validation of layout qualifiers specified in
   * redeclaration of gl_FragCoord for the attached fragment shaders,
   * and propagates them to the linked FS and linked shader program.
   */
   static void
   link_fs_inout_layout_qualifiers(struct gl_shader_program *prog,
                           {
      bool redeclares_gl_fragcoord = false;
   bool uses_gl_fragcoord = false;
   bool origin_upper_left = false;
            if (linked_shader->Stage != MESA_SHADER_FRAGMENT ||
      (prog->GLSL_Version < 150 && !arb_fragment_coord_conventions_enable))
         for (unsigned i = 0; i < num_shaders; i++) {
      struct gl_shader *shader = shader_list[i];
   /* From the GLSL 1.50 spec, page 39:
   *
   *   "If gl_FragCoord is redeclared in any fragment shader in a program,
   *    it must be redeclared in all the fragment shaders in that program
   *    that have a static use gl_FragCoord."
   */
   if ((redeclares_gl_fragcoord && !shader->redeclares_gl_fragcoord &&
      shader->uses_gl_fragcoord)
   || (shader->redeclares_gl_fragcoord && !redeclares_gl_fragcoord &&
      uses_gl_fragcoord)) {
   linker_error(prog, "fragment shader defined with conflicting "
            /* From the GLSL 1.50 spec, page 39:
   *
   *   "All redeclarations of gl_FragCoord in all fragment shaders in a
   *    single program must have the same set of qualifiers."
   */
   if (redeclares_gl_fragcoord && shader->redeclares_gl_fragcoord &&
      (shader->origin_upper_left != origin_upper_left ||
   shader->pixel_center_integer != pixel_center_integer)) {
   linker_error(prog, "fragment shader defined with conflicting "
               /* Update the linked shader state.  Note that uses_gl_fragcoord should
   * accumulate the results.  The other values should replace.  If there
   * are multiple redeclarations, all the fields except uses_gl_fragcoord
   * are already known to be the same.
   */
   if (shader->redeclares_gl_fragcoord || shader->uses_gl_fragcoord) {
      redeclares_gl_fragcoord = shader->redeclares_gl_fragcoord;
   uses_gl_fragcoord |= shader->uses_gl_fragcoord;
   origin_upper_left = shader->origin_upper_left;
               linked_shader->Program->info.fs.early_fragment_tests |=
         linked_shader->Program->info.fs.inner_coverage |= shader->InnerCoverage;
   linked_shader->Program->info.fs.post_depth_coverage |=
         linked_shader->Program->info.fs.pixel_interlock_ordered |=
         linked_shader->Program->info.fs.pixel_interlock_unordered |=
         linked_shader->Program->info.fs.sample_interlock_ordered |=
         linked_shader->Program->info.fs.sample_interlock_unordered |=
                     linked_shader->Program->info.fs.pixel_center_integer = pixel_center_integer;
      }
      /**
   * Performs the cross-validation of geometry shader max_vertices and
   * primitive type layout qualifiers for the attached geometry shaders,
   * and propagates them to the linked GS and linked shader program.
   */
   static void
   link_gs_inout_layout_qualifiers(struct gl_shader_program *prog,
                     {
      /* No in/out qualifiers defined for anything but GLSL 1.50+
   * geometry shaders so far.
   */
   if (gl_prog->info.stage != MESA_SHADER_GEOMETRY || prog->GLSL_Version < 150)
                     gl_prog->info.gs.invocations = 0;
   gl_prog->info.gs.input_primitive = MESA_PRIM_UNKNOWN;
            /* From the GLSL 1.50 spec, page 46:
   *
   *     "All geometry shader output layout declarations in a program
   *      must declare the same layout and same value for
   *      max_vertices. There must be at least one geometry output
   *      layout declaration somewhere in a program, but not all
   *      geometry shaders (compilation units) are required to
   *      declare it."
            for (unsigned i = 0; i < num_shaders; i++) {
               if (shader->info.Geom.InputType != MESA_PRIM_UNKNOWN) {
      if (gl_prog->info.gs.input_primitive != MESA_PRIM_UNKNOWN &&
      gl_prog->info.gs.input_primitive !=
   shader->info.Geom.InputType) {
   linker_error(prog, "geometry shader defined with conflicting "
            }
               if (shader->info.Geom.OutputType != MESA_PRIM_UNKNOWN) {
      if (gl_prog->info.gs.output_primitive != MESA_PRIM_UNKNOWN &&
      gl_prog->info.gs.output_primitive !=
   shader->info.Geom.OutputType) {
   linker_error(prog, "geometry shader defined with conflicting "
            }
               if (shader->info.Geom.VerticesOut != -1) {
      if (vertices_out != -1 &&
      vertices_out != shader->info.Geom.VerticesOut) {
   linker_error(prog, "geometry shader defined with conflicting "
                  }
               if (shader->info.Geom.Invocations != 0) {
      if (gl_prog->info.gs.invocations != 0 &&
      gl_prog->info.gs.invocations !=
   (unsigned) shader->info.Geom.Invocations) {
   linker_error(prog, "geometry shader defined with conflicting "
               "invocation count (%d and %d)\n",
      }
                  /* Just do the intrastage -> interstage propagation right now,
   * since we already know we're in the right type of shader program
   * for doing it.
   */
   if (gl_prog->info.gs.input_primitive == MESA_PRIM_UNKNOWN) {
      linker_error(prog,
                     if (gl_prog->info.gs.output_primitive == MESA_PRIM_UNKNOWN) {
      linker_error(prog,
                     if (vertices_out == -1) {
      linker_error(prog,
            } else {
                  if (gl_prog->info.gs.invocations == 0)
      }
         /**
   * Perform cross-validation of compute shader local_size_{x,y,z} layout and
   * derivative arrangement qualifiers for the attached compute shaders, and
   * propagate them to the linked CS and linked shader program.
   */
   static void
   link_cs_input_layout_qualifiers(struct gl_shader_program *prog,
                     {
      /* This function is called for all shader stages, but it only has an effect
   * for compute shaders.
   */
   if (gl_prog->info.stage != MESA_SHADER_COMPUTE)
            for (int i = 0; i < 3; i++)
                              /* From the ARB_compute_shader spec, in the section describing local size
   * declarations:
   *
   *     If multiple compute shaders attached to a single program object
   *     declare local work-group size, the declarations must be identical;
   *     otherwise a link-time error results. Furthermore, if a program
   *     object contains any compute shaders, at least one must contain an
   *     input layout qualifier specifying the local work sizes of the
   *     program, or a link-time error will occur.
   */
   for (unsigned sh = 0; sh < num_shaders; sh++) {
               if (shader->info.Comp.LocalSize[0] != 0) {
      if (gl_prog->info.workgroup_size[0] != 0) {
      for (int i = 0; i < 3; i++) {
      if (gl_prog->info.workgroup_size[i] !=
      shader->info.Comp.LocalSize[i]) {
   linker_error(prog, "compute shader defined with conflicting "
                  }
   for (int i = 0; i < 3; i++) {
      gl_prog->info.workgroup_size[i] =
         } else if (shader->info.Comp.LocalSizeVariable) {
      if (gl_prog->info.workgroup_size[0] != 0) {
      /* The ARB_compute_variable_group_size spec says:
   *
   *     If one compute shader attached to a program declares a
   *     variable local group size and a second compute shader
   *     attached to the same program declares a fixed local group
   *     size, a link-time error results.
   */
   linker_error(prog, "compute shader defined with both fixed and "
            }
               enum gl_derivative_group group = shader->info.Comp.DerivativeGroup;
   if (group != DERIVATIVE_GROUP_NONE) {
      if (gl_prog->info.cs.derivative_group != DERIVATIVE_GROUP_NONE &&
      gl_prog->info.cs.derivative_group != group) {
   linker_error(prog, "compute shader defined with conflicting "
            }
                  /* Just do the intrastage -> interstage propagation right now,
   * since we already know we're in the right type of shader program
   * for doing it.
   */
   if (gl_prog->info.workgroup_size[0] == 0 &&
      !gl_prog->info.workgroup_size_variable) {
   linker_error(prog, "compute shader must contain a fixed or a variable "
                     if (gl_prog->info.cs.derivative_group == DERIVATIVE_GROUP_QUADS) {
      if (gl_prog->info.workgroup_size[0] % 2 != 0) {
      linker_error(prog, "derivative_group_quadsNV must be used with a "
                  }
   if (gl_prog->info.workgroup_size[1] % 2 != 0) {
      linker_error(prog, "derivative_group_quadsNV must be used with a local"
                     } else if (gl_prog->info.cs.derivative_group == DERIVATIVE_GROUP_LINEAR) {
      if ((gl_prog->info.workgroup_size[0] *
      gl_prog->info.workgroup_size[1] *
   gl_prog->info.workgroup_size[2]) % 4 != 0) {
   linker_error(prog, "derivative_group_linearNV must be used with a "
                        }
      /**
   * Link all out variables on a single stage which are not
   * directly used in a shader with the main function.
   */
   static void
   link_output_variables(struct gl_linked_shader *linked_shader,
               {
                           /* Skip shader object with main function */
   if (shader_list[i]->symbols->get_function("main"))
            foreach_in_list(ir_instruction, ir, shader_list[i]->ir) {
                              if (var->data.mode == ir_var_shader_out &&
            var = var->clone(linked_shader, NULL);
   symbols->add_variable(var);
                        }
         /**
   * Combine a group of shaders for a single stage to generate a linked shader
   *
   * \note
   * If this function is supplied a single shader, it is cloned, and the new
   * shader is returned.
   */
   struct gl_linked_shader *
   link_intrastage_shaders(void *mem_ctx,
                           struct gl_context *ctx,
   {
      struct gl_uniform_block *ubo_blocks = NULL;
   struct gl_uniform_block *ssbo_blocks = NULL;
   unsigned num_ubo_blocks = 0;
   unsigned num_ssbo_blocks = 0;
            /* Check that global variables defined in multiple shaders are consistent.
   */
   glsl_symbol_table variables;
   for (unsigned i = 0; i < num_shaders; i++) {
      if (shader_list[i] == NULL)
         cross_validate_globals(&ctx->Const, prog, shader_list[i]->ir, &variables,
         if (shader_list[i]->ARB_fragment_coord_conventions_enable)
               if (!prog->data->LinkStatus)
            /* Check that interface blocks defined in multiple shaders are consistent.
   */
   validate_intrastage_interface_blocks(prog, (const gl_shader **)shader_list,
         if (!prog->data->LinkStatus)
            /* Check that there is only a single definition of each function signature
   * across all shaders.
   */
   for (unsigned i = 0; i < (num_shaders - 1); i++) {
      foreach_in_list(ir_instruction, node, shader_list[i]->ir) {
                              for (unsigned j = i + 1; j < num_shaders; j++) {
                     /* If the other shader has no function (and therefore no function
   * signatures) with the same name, skip to the next shader.
   */
                  foreach_in_list(ir_function_signature, sig, &f->signatures) {
                                    if (other_sig != NULL && other_sig->is_defined) {
      linker_error(prog, "function `%s' is multiply defined\n",
                                 /* Find the shader that defines main, and make a clone of it.
   *
   * Starting with the clone, search for undefined references.  If one is
   * found, find the shader that defines it.  Clone the reference and add
   * it to the shader.  Repeat until there are no undefined references or
   * until a reference cannot be resolved.
   */
   gl_shader *main = NULL;
   for (unsigned i = 0; i < num_shaders; i++) {
      if (_mesa_get_main_function_signature(shader_list[i]->symbols)) {
      main = shader_list[i];
                  if (main == NULL && allow_missing_main)
            if (main == NULL) {
      linker_error(prog, "%s shader lacks `main'\n",
                     gl_linked_shader *linked = rzalloc(NULL, struct gl_linked_shader);
            /* Create program and attach it to the linked shader */
   struct gl_program *gl_prog =
         if (!gl_prog) {
      prog->data->LinkStatus = LINKING_FAILURE;
   _mesa_delete_linked_shader(ctx, linked);
                        /* Don't use _mesa_reference_program() just take ownership */
            linked->ir = new(linked) exec_list;
            link_fs_inout_layout_qualifiers(prog, linked, shader_list, num_shaders,
         link_tcs_out_layout_qualifiers(prog, gl_prog, shader_list, num_shaders);
   link_tes_in_layout_qualifiers(prog, gl_prog, shader_list, num_shaders);
   link_gs_inout_layout_qualifiers(prog, gl_prog, shader_list, num_shaders);
            if (linked->Stage != MESA_SHADER_FRAGMENT)
                                       /* The pointer to the main function in the final linked shader (i.e., the
   * copy of the original shader that contained the main function).
   */
   ir_function_signature *const main_sig =
            /* Move any instructions other than variable declarations or function
   * declarations into main.
   */
   if (main_sig != NULL) {
      exec_node *insertion_point =
                  for (unsigned i = 0; i < num_shaders; i++) {
                     insertion_point = move_non_declarations(shader_list[i]->ir,
                  if (!link_function_calls(prog, linked, shader_list, num_shaders)) {
      _mesa_delete_linked_shader(ctx, linked);
               if (linked->Stage != MESA_SHADER_FRAGMENT)
            /* Make a pass over all variable declarations to ensure that arrays with
   * unspecified sizes have a size specified.  The size is inferred from the
   * max_array_access field.
   */
   array_sizing_visitor v;
   v.run(linked->ir);
            /* Now that we know the sizes of all the arrays, we can replace .length()
   * calls with a constant expression.
   */
   array_length_to_const_visitor len_v;
            /* Link up uniform blocks defined within this stage. */
   link_uniform_blocks(mem_ctx, &ctx->Const, prog, linked, &ubo_blocks,
            const unsigned max_uniform_blocks =
         if (num_ubo_blocks > max_uniform_blocks) {
      linker_error(prog, "Too many %s uniform blocks (%d/%d)\n",
                     const unsigned max_shader_storage_blocks =
         if (num_ssbo_blocks > max_shader_storage_blocks) {
      linker_error(prog, "Too many %s shader storage blocks (%d/%d)\n",
                     if (!prog->data->LinkStatus) {
      _mesa_delete_linked_shader(ctx, linked);
               /* Copy ubo blocks to linked shader list */
   linked->Program->sh.UniformBlocks =
         ralloc_steal(linked, ubo_blocks);
   for (unsigned i = 0; i < num_ubo_blocks; i++) {
         }
   linked->Program->sh.NumUniformBlocks = num_ubo_blocks;
            /* Copy ssbo blocks to linked shader list */
   linked->Program->sh.ShaderStorageBlocks =
         ralloc_steal(linked, ssbo_blocks);
   for (unsigned i = 0; i < num_ssbo_blocks; i++) {
         }
            /* At this point linked should contain all of the linked IR, so
   * validate it to make sure nothing went wrong.
   */
            /* Set the size of geometry shader input arrays */
   if (linked->Stage == MESA_SHADER_GEOMETRY) {
      unsigned num_vertices =
         array_resize_visitor input_resize_visitor(num_vertices, prog,
         foreach_in_list(ir_instruction, ir, linked->ir) {
                     /* Set the linked source SHA1. */
   if (num_shaders == 1) {
      memcpy(linked->linked_source_sha1, shader_list[0]->compiled_source_sha1,
      } else {
      struct mesa_sha1 sha1_ctx;
            for (unsigned i = 0; i < num_shaders; i++) {
                     _mesa_sha1_update(&sha1_ctx, shader_list[i]->compiled_source_sha1,
      }
                  }
      /**
   * Resize tessellation evaluation per-vertex inputs to the size of
   * tessellation control per-vertex outputs.
   */
   static void
   resize_tes_inputs(const struct gl_constants *consts,
         {
      if (prog->_LinkedShaders[MESA_SHADER_TESS_EVAL] == NULL)
            gl_linked_shader *const tcs = prog->_LinkedShaders[MESA_SHADER_TESS_CTRL];
            /* If no control shader is present, then the TES inputs are statically
   * sized to MaxPatchVertices; the actual size of the arrays won't be
   * known until draw time.
   */
   const int num_vertices = tcs
      ? tcs->Program->info.tess.tcs_vertices_out
         array_resize_visitor input_resize_visitor(num_vertices, prog,
         foreach_in_list(ir_instruction, ir, tes->ir) {
                  if (tcs) {
      /* Convert the gl_PatchVerticesIn system value into a constant, since
   * the value is known at this point.
   */
   foreach_in_list(ir_instruction, ir, tes->ir) {
      ir_variable *var = ir->as_variable();
   if (var && var->data.mode == ir_var_system_value &&
      var->data.location == SYSTEM_VALUE_VERTICES_IN) {
   void *mem_ctx = ralloc_parent(var);
   var->data.location = 0;
   var->data.explicit_location = false;
   var->data.mode = ir_var_auto;
               }
         /**
   * Initializes explicit location slots to INACTIVE_UNIFORM_EXPLICIT_LOCATION
   * for a variable, checks for overlaps between other uniforms using explicit
   * locations.
   */
   static int
   reserve_explicit_locations(struct gl_shader_program *prog,
         {
      unsigned slots = var->type->uniform_locations();
   unsigned max_loc = var->data.location + slots - 1;
            /* Resize remap table if locations do not fit in the current one. */
   if (max_loc + 1 > prog->NumUniformRemapTable) {
      prog->UniformRemapTable =
      reralloc(prog, prog->UniformRemapTable,
               if (!prog->UniformRemapTable) {
      linker_error(prog, "Out of memory during linking.\n");
               /* Initialize allocated space. */
   for (unsigned i = prog->NumUniformRemapTable; i < max_loc + 1; i++)
                        for (unsigned i = 0; i < slots; i++) {
               /* Check if location is already used. */
               /* Possibly same uniform from a different stage, this is ok. */
   unsigned hash_loc;
   if (map->get(hash_loc, var->name) && hash_loc == loc - i) {
      return_value = 0;
               /* ARB_explicit_uniform_location specification states:
   *
   *     "No two default-block uniform variables in the program can have
   *     the same location, even if they are unused, otherwise a compiler
   *     or linker error will be generated."
   */
   linker_error(prog,
               "location qualifier for uniform %s overlaps "
               /* Initialize location as inactive before optimization
   * rounds and location assignment.
   */
               /* Note, base location used for arrays. */
               }
      static bool
   reserve_subroutine_explicit_locations(struct gl_shader_program *prog,
               {
      unsigned slots = var->type->uniform_locations();
            /* Resize remap table if locations do not fit in the current one. */
   if (max_loc + 1 > p->sh.NumSubroutineUniformRemapTable) {
      p->sh.SubroutineUniformRemapTable =
      reralloc(p, p->sh.SubroutineUniformRemapTable,
               if (!p->sh.SubroutineUniformRemapTable) {
      linker_error(prog, "Out of memory during linking.\n");
               /* Initialize allocated space. */
   for (unsigned i = p->sh.NumSubroutineUniformRemapTable; i < max_loc + 1; i++)
                        for (unsigned i = 0; i < slots; i++) {
               /* Check if location is already used. */
               /* ARB_explicit_uniform_location specification states:
   *     "No two subroutine uniform variables can have the same location
   *     in the same shader stage, otherwise a compiler or linker error
   *     will be generated."
   */
   linker_error(prog,
               "location qualifier for uniform %s overlaps "
               /* Initialize location as inactive before optimization
   * rounds and location assignment.
   */
                  }
   /**
   * Check and reserve all explicit uniform locations, called before
   * any optimizations happen to handle also inactive uniforms and
   * inactive array elements that may get trimmed away.
   */
   static void
   check_explicit_uniform_locations(const struct gl_extensions *exts,
         {
               if (!exts->ARB_explicit_uniform_location)
            /* This map is used to detect if overlapping explicit locations
   * occur with the same uniform (from different stage) or a different one.
   */
            if (!uniform_map) {
      linker_error(prog, "Out of memory during linking.\n");
               unsigned entries_total = 0;
   unsigned mask = prog->data->linked_stages;
   while (mask) {
      const int i = u_bit_scan(&mask);
            foreach_in_list(ir_instruction, node, prog->_LinkedShaders[i]->ir) {
      ir_variable *var = node->as_variable();
                  if (var->data.explicit_location) {
      bool ret = false;
   if (var->type->without_array()->is_subroutine())
         else {
      int slots = reserve_explicit_locations(prog, uniform_map,
         if (slots != -1) {
      ret = true;
         }
   if (!ret) {
      delete uniform_map;
                                 delete uniform_map;
      }
      static void
   link_assign_subroutine_types(struct gl_shader_program *prog)
   {
      unsigned mask = prog->data->linked_stages;
   while (mask) {
      const int i = u_bit_scan(&mask);
            p->sh.MaxSubroutineFunctionIndex = 0;
   foreach_in_list(ir_instruction, node, prog->_LinkedShaders[i]->ir) {
      ir_function *fn = node->as_function();
                                                /* these should have been calculated earlier. */
   assert(fn->subroutine_index != -1);
   if (p->sh.NumSubroutineFunctions + 1 > MAX_SUBROUTINES) {
      linker_error(prog, "Too many subroutine functions declared.\n");
      }
   p->sh.SubroutineFunctions = reralloc(p, p->sh.SubroutineFunctions,
               p->sh.SubroutineFunctions[p->sh.NumSubroutineFunctions].name.string = ralloc_strdup(p, fn->name);
   resource_name_updated(&p->sh.SubroutineFunctions[p->sh.NumSubroutineFunctions].name);
   p->sh.SubroutineFunctions[p->sh.NumSubroutineFunctions].num_compat_types = fn->num_subroutine_types;
   p->sh.SubroutineFunctions[p->sh.NumSubroutineFunctions].types =
                  /* From Section 4.4.4(Subroutine Function Layout Qualifiers) of the
   * GLSL 4.5 spec:
   *
   *    "Each subroutine with an index qualifier in the shader must be
   *    given a unique index, otherwise a compile or link error will be
   *    generated."
   */
   for (unsigned j = 0; j < p->sh.NumSubroutineFunctions; j++) {
      if (p->sh.SubroutineFunctions[j].index != -1 &&
      p->sh.SubroutineFunctions[j].index == fn->subroutine_index) {
   linker_error(prog, "each subroutine index qualifier in the "
               }
                                 for (int j = 0; j < fn->num_subroutine_types; j++)
                  }
      static void
   verify_subroutine_associated_funcs(struct gl_shader_program *prog)
   {
      unsigned mask = prog->data->linked_stages;
   while (mask) {
      const int i = u_bit_scan(&mask);
   gl_program *p = prog->_LinkedShaders[i]->Program;
            /* Section 6.1.2 (Subroutines) of the GLSL 4.00 spec says:
   *
   *   "A program will fail to compile or link if any shader
   *    or stage contains two or more functions with the same
   *    name if the name is associated with a subroutine type."
   */
   for (unsigned j = 0; j < p->sh.NumSubroutineFunctions; j++) {
      unsigned definitions = 0;
                  /* Calculate number of function definitions with the same name */
   foreach_in_list(ir_function_signature, sig, &fn->signatures) {
      if (sig->is_defined) {
      if (++definitions > 1) {
      linker_error(prog, "%s shader contains two or more function "
               "definitions with name `%s', which is "
   "associated with a subroutine type.\n",
                     }
      void
   link_shaders(struct gl_context *ctx, struct gl_shader_program *prog)
   {
      const struct gl_constants *consts = &ctx->Const;
   prog->data->LinkStatus = LINKING_SUCCESS; /* All error paths will set this to false */
            /* Section 7.3 (Program Objects) of the OpenGL 4.5 Core Profile spec says:
   *
   *     "Linking can fail for a variety of reasons as specified in the
   *     OpenGL Shading Language Specification, as well as any of the
   *     following reasons:
   *
   *     - No shader objects are attached to program."
   *
   * The Compatibility Profile specification does not list the error.  In
   * Compatibility Profile missing shader stages are replaced by
   * fixed-function.  This applies to the case where all stages are
   * missing.
   */
   if (prog->NumShaders == 0) {
      if (ctx->API != API_OPENGL_COMPAT)
                  #ifdef ENABLE_SHADER_CACHE
      if (shader_cache_read_program_metadata(ctx, prog))
      #endif
                  /* Separate the shaders into groups based on their type.
   */
   struct gl_shader **shader_list[MESA_SHADER_STAGES];
            for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      shader_list[i] = (struct gl_shader **)
                     unsigned min_version = UINT_MAX;
   unsigned max_version = 0;
   for (unsigned i = 0; i < prog->NumShaders; i++) {
      min_version = MIN2(min_version, prog->Shaders[i]->Version);
            if (!consts->AllowGLSLRelaxedES &&
      prog->Shaders[i]->IsES != prog->Shaders[0]->IsES) {
   linker_error(prog, "all shaders must use same shading "
                     gl_shader_stage shader_type = prog->Shaders[i]->Stage;
   shader_list[shader_type][num_shaders[shader_type]] = prog->Shaders[i];
               /* In desktop GLSL, different shader versions may be linked together.  In
   * GLSL ES, all shader versions must be the same.
   */
   if (!consts->AllowGLSLRelaxedES && prog->Shaders[0]->IsES &&
      min_version != max_version) {
   linker_error(prog, "all shaders must use same shading "
                     prog->GLSL_Version = max_version;
            /* Some shaders have to be linked with some other shaders present.
   */
   if (!prog->SeparateShader) {
      if (num_shaders[MESA_SHADER_GEOMETRY] > 0 &&
      num_shaders[MESA_SHADER_VERTEX] == 0) {
   linker_error(prog, "Geometry shader must be linked with "
            }
   if (num_shaders[MESA_SHADER_TESS_EVAL] > 0 &&
      num_shaders[MESA_SHADER_VERTEX] == 0) {
   linker_error(prog, "Tessellation evaluation shader must be linked "
            }
   if (num_shaders[MESA_SHADER_TESS_CTRL] > 0 &&
      num_shaders[MESA_SHADER_VERTEX] == 0) {
   linker_error(prog, "Tessellation control shader must be linked with "
                     /* Section 7.3 of the OpenGL ES 3.2 specification says:
   *
   *    "Linking can fail for [...] any of the following reasons:
   *
   *     * program contains an object to form a tessellation control
   *       shader [...] and [...] the program is not separable and
   *       contains no object to form a tessellation evaluation shader"
   *
   * The OpenGL spec is contradictory. It allows linking without a tess
   * eval shader, but that can only be used with transform feedback and
   * rasterization disabled. However, transform feedback isn't allowed
   * with GL_PATCHES, so it can't be used.
   *
   * More investigation showed that the idea of transform feedback after
   * a tess control shader was dropped, because some hw vendors couldn't
   * support tessellation without a tess eval shader, but the linker
   * section wasn't updated to reflect that.
   *
   * All specifications (ARB_tessellation_shader, GL 4.0-4.5) have this
   * spec bug.
   *
   * Do what's reasonable and always require a tess eval shader if a tess
   * control shader is present.
   */
   if (num_shaders[MESA_SHADER_TESS_CTRL] > 0 &&
      num_shaders[MESA_SHADER_TESS_EVAL] == 0) {
   linker_error(prog, "Tessellation control shader must be linked with "
                     if (prog->IsES) {
      if (num_shaders[MESA_SHADER_TESS_EVAL] > 0 &&
      num_shaders[MESA_SHADER_TESS_CTRL] == 0) {
   linker_error(prog, "GLSL ES requires non-separable programs "
                                 /* Compute shaders have additional restrictions. */
   if (num_shaders[MESA_SHADER_COMPUTE] > 0 &&
      num_shaders[MESA_SHADER_COMPUTE] != prog->NumShaders) {
   linker_error(prog, "Compute shaders may not be linked with any other "
               /* Link all shaders for a particular stage and validate the result.
   */
   for (int stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (num_shaders[stage] > 0) {
      gl_linked_shader *const sh =
                  if (!prog->data->LinkStatus) {
      if (sh)
                     switch (stage) {
   case MESA_SHADER_VERTEX:
      validate_vertex_shader_executable(prog, sh, consts);
      case MESA_SHADER_TESS_CTRL:
      /* nothing to be done */
      case MESA_SHADER_TESS_EVAL:
      validate_tess_eval_shader_executable(prog, sh, consts);
      case MESA_SHADER_GEOMETRY:
      validate_geometry_shader_executable(prog, sh, consts);
      case MESA_SHADER_FRAGMENT:
      validate_fragment_shader_executable(prog, sh);
      }
   if (!prog->data->LinkStatus) {
      if (sh)
                     prog->_LinkedShaders[stage] = sh;
                  /* Here begins the inter-stage linking phase.  Some initial validation is
   * performed, then locations are assigned for uniforms, attributes, and
   * varyings.
   */
   cross_validate_uniforms(consts, prog);
   if (!prog->data->LinkStatus)
                     first = MESA_SHADER_STAGES;
   /* Determine first and last stage. */
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (!prog->_LinkedShaders[i])
         if (first == MESA_SHADER_STAGES)
               check_explicit_uniform_locations(&ctx->Extensions, prog);
   link_assign_subroutine_types(prog);
            if (!prog->data->LinkStatus)
                     /* Validate the inputs of each stage with the output of the preceding
   * stage.
   */
   prev = first;
   for (unsigned i = prev + 1; i <= MESA_SHADER_FRAGMENT; i++) {
      if (prog->_LinkedShaders[i] == NULL)
            validate_interstage_inout_blocks(prog, prog->_LinkedShaders[prev],
         if (!prog->data->LinkStatus)
                        /* Cross-validate uniform blocks between shader stages */
   validate_interstage_uniform_blocks(prog, prog->_LinkedShaders);
   if (!prog->data->LinkStatus)
            for (unsigned int i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i] != NULL)
               if (prog->IsES && prog->GLSL_Version == 100)
      if (!validate_invariant_builtins(prog,
         prog->_LinkedShaders[MESA_SHADER_VERTEX],
         /* Implement the GLSL 1.30+ rule for discard vs infinite loops Do
   * it before optimization because we want most of the checks to get
   * dropped thanks to constant propagation.
   *
   * This rule also applies to GLSL ES 3.00.
   */
   if (max_version >= (prog->IsES ? 300 : 130)) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[MESA_SHADER_FRAGMENT];
   if (sh) {
                     /* Process UBOs */
   if (!interstage_cross_validate_uniform_blocks(prog, false))
            /* Process SSBOs */
   if (!interstage_cross_validate_uniform_blocks(prog, true))
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
            detect_recursion_linked(prog, prog->_LinkedShaders[i]->ir);
   if (!prog->data->LinkStatus)
            if (consts->ShaderCompilerOptions[i].LowerCombinedClipCullDistance) {
                     /* Check and validate stream emissions in geometry shaders */
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
                     done:
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      free(shader_list[i]);
   if (prog->_LinkedShaders[i] == NULL)
            /* Do a final validation step to make sure that the IR wasn't
   * invalidated by any modifications performed after intrastage linking.
   */
            /* Retain any live IR, but trash the rest. */
            /* The symbol table in the linked shaders may contain references to
   * variables that were removed (e.g., unused uniforms).  Since it may
   * contain junk, there is no possible valid use.  Delete it and set the
   * pointer to NULL.
   */
   delete prog->_LinkedShaders[i]->symbols;
                  }
      void
   resource_name_updated(struct gl_resource_name *name)
   {
      if (name->string) {
               const char *last_square_bracket = strrchr(name->string, '[');
   if (last_square_bracket) {
      name->last_square_bracket = last_square_bracket - name->string;
   name->suffix_is_zero_square_bracketed =
      } else {
      name->last_square_bracket = -1;
         } else {
      name->length = 0;
   name->last_square_bracket = -1;
         }
