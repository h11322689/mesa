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
      #include "ast.h"
   #include "util/string_buffer.h"
      void
   ast_type_specifier::print(void) const
   {
      if (structure) {
         } else {
                  if (array_specifier) {
            }
      bool
   ast_fully_specified_type::has_qualifiers(_mesa_glsl_parse_state *state) const
   {
      /* 'subroutine' isnt a real qualifier. */
   ast_type_qualifier subroutine_only;
   subroutine_only.flags.i = 0;
   subroutine_only.flags.q.subroutine = 1;
   if (state->has_explicit_uniform_location()) {
         }
      }
      bool ast_type_qualifier::has_interpolation() const
   {
      return this->flags.q.smooth
            }
      bool
   ast_type_qualifier::has_layout() const
   {
      return this->flags.q.origin_upper_left
         || this->flags.q.pixel_center_integer
   || this->flags.q.depth_type
   || this->flags.q.std140
   || this->flags.q.std430
   || this->flags.q.shared
   || this->flags.q.column_major
   || this->flags.q.row_major
   || this->flags.q.packed
   || this->flags.q.bindless_sampler
   || this->flags.q.bindless_image
   || this->flags.q.bound_sampler
   || this->flags.q.bound_image
   || this->flags.q.explicit_align
   || this->flags.q.explicit_component
   || this->flags.q.explicit_location
   || this->flags.q.explicit_image_format
   || this->flags.q.explicit_index
   || this->flags.q.explicit_binding
   || this->flags.q.explicit_offset
   || this->flags.q.explicit_stream
   || this->flags.q.explicit_xfb_buffer
      }
      bool
   ast_type_qualifier::has_storage() const
   {
      return this->flags.q.constant
         || this->flags.q.attribute
   || this->flags.q.varying
   || this->flags.q.in
   || this->flags.q.out
   || this->flags.q.uniform
      }
      bool
   ast_type_qualifier::has_auxiliary_storage() const
   {
      return this->flags.q.centroid
            }
      bool ast_type_qualifier::has_memory() const
   {
      return this->flags.q.coherent
         || this->flags.q._volatile
   || this->flags.q.restrict_flag
      }
      bool ast_type_qualifier::is_subroutine_decl() const
   {
         }
      static bool
   validate_prim_type(YYLTYPE *loc,
                     {
      /* Input layout qualifiers can be specified multiple
   * times in separate declarations, as long as they match.
   */
   if (qualifier.flags.q.prim_type && new_qualifier.flags.q.prim_type
      && qualifier.prim_type != new_qualifier.prim_type) {
   _mesa_glsl_error(loc, state,
                                    }
      static bool
   validate_vertex_spacing(YYLTYPE *loc,
                     {
      if (qualifier.flags.q.vertex_spacing && new_qualifier.flags.q.vertex_spacing
      && qualifier.vertex_spacing != new_qualifier.vertex_spacing) {
   _mesa_glsl_error(loc, state,
                        }
      static bool
   validate_ordering(YYLTYPE *loc,
                     {
      if (qualifier.flags.q.ordering && new_qualifier.flags.q.ordering
      && qualifier.ordering != new_qualifier.ordering) {
   _mesa_glsl_error(loc, state,
                        }
      static bool
   validate_point_mode(ASSERTED const ast_type_qualifier &qualifier,
         {
      /* Point mode can only be true if the flag is set. */
   assert (!qualifier.flags.q.point_mode || !new_qualifier.flags.q.point_mode
               }
      static void
   merge_bindless_qualifier(_mesa_glsl_parse_state *state)
   {
      if (state->default_uniform_qualifier->flags.q.bindless_sampler) {
      state->bindless_sampler_specified = true;
               if (state->default_uniform_qualifier->flags.q.bindless_image) {
      state->bindless_image_specified = true;
               if (state->default_uniform_qualifier->flags.q.bound_sampler) {
      state->bound_sampler_specified = true;
               if (state->default_uniform_qualifier->flags.q.bound_image) {
      state->bound_image_specified = true;
         }
      /**
   * This function merges duplicate layout identifiers.
   *
   * It deals with duplicates within a single layout qualifier, among multiple
   * layout qualifiers on a single declaration and on several declarations for
   * the same variable.
   *
   * The is_single_layout_merge and is_multiple_layouts_merge parameters are
   * used to differentiate among them.
   */
   bool
   ast_type_qualifier::merge_qualifier(YYLTYPE *loc,
                           {
      bool r = true;
   ast_type_qualifier ubo_mat_mask;
   ubo_mat_mask.flags.i = 0;
   ubo_mat_mask.flags.q.row_major = 1;
            ast_type_qualifier ubo_layout_mask;
   ubo_layout_mask.flags.i = 0;
   ubo_layout_mask.flags.q.std140 = 1;
   ubo_layout_mask.flags.q.packed = 1;
   ubo_layout_mask.flags.q.shared = 1;
            ast_type_qualifier ubo_binding_mask;
   ubo_binding_mask.flags.i = 0;
   ubo_binding_mask.flags.q.explicit_binding = 1;
            ast_type_qualifier stream_layout_mask;
   stream_layout_mask.flags.i = 0;
            /* FIXME: We should probably do interface and function param validation
   * separately.
   */
   ast_type_qualifier input_layout_mask;
   input_layout_mask.flags.i = 0;
   input_layout_mask.flags.q.centroid = 1;
   /* Function params can have constant */
   input_layout_mask.flags.q.constant = 1;
   input_layout_mask.flags.q.explicit_component = 1;
   input_layout_mask.flags.q.explicit_location = 1;
   input_layout_mask.flags.q.flat = 1;
   input_layout_mask.flags.q.in = 1;
   input_layout_mask.flags.q.invariant = 1;
   input_layout_mask.flags.q.noperspective = 1;
   input_layout_mask.flags.q.origin_upper_left = 1;
   /* Function params 'inout' will set this */
   input_layout_mask.flags.q.out = 1;
   input_layout_mask.flags.q.patch = 1;
   input_layout_mask.flags.q.pixel_center_integer = 1;
   input_layout_mask.flags.q.precise = 1;
   input_layout_mask.flags.q.sample = 1;
   input_layout_mask.flags.q.smooth = 1;
            if (state->has_bindless()) {
      /* Allow to use image qualifiers with shader inputs/outputs. */
   input_layout_mask.flags.q.coherent = 1;
   input_layout_mask.flags.q._volatile = 1;
   input_layout_mask.flags.q.restrict_flag = 1;
   input_layout_mask.flags.q.read_only = 1;
   input_layout_mask.flags.q.write_only = 1;
               /* Uniform block layout qualifiers get to overwrite each
   * other (rightmost having priority), while all other
   * qualifiers currently don't allow duplicates.
   */
   ast_type_qualifier allowed_duplicates_mask;
   allowed_duplicates_mask.flags.i =
      ubo_mat_mask.flags.i |
   ubo_layout_mask.flags.i |
         /* Geometry shaders can have several layout qualifiers
   * assigning different stream values.
   */
   if (state->stage == MESA_SHADER_GEOMETRY) {
      allowed_duplicates_mask.flags.i |=
               if (is_single_layout_merge && !state->has_enhanced_layouts() &&
      (this->flags.i & q.flags.i & ~allowed_duplicates_mask.flags.i) != 0) {
   _mesa_glsl_error(loc, state, "duplicate layout qualifiers used");
               if (is_multiple_layouts_merge && !state->has_420pack_or_es31()) {
      _mesa_glsl_error(loc, state,
                     if (q.flags.q.prim_type) {
      r &= validate_prim_type(loc, state, *this, q);
   this->flags.q.prim_type = 1;
               if (q.flags.q.max_vertices) {
      if (this->flags.q.max_vertices
      && !is_single_layout_merge && !is_multiple_layouts_merge) {
      } else {
      this->flags.q.max_vertices = 1;
                  if (q.subroutine_list) {
      if (this->subroutine_list) {
      _mesa_glsl_error(loc, state,
      } else {
                     if (q.flags.q.invocations) {
      if (this->flags.q.invocations
      && !is_single_layout_merge && !is_multiple_layouts_merge) {
      } else {
      this->flags.q.invocations = 1;
                  if (state->stage == MESA_SHADER_GEOMETRY &&
      state->has_explicit_attrib_stream()) {
   if (!this->flags.q.explicit_stream) {
      if (q.flags.q.stream) {
      this->flags.q.stream = 1;
      } else if (!this->flags.q.stream && this->flags.q.out &&
            /* Assign default global stream value */
   this->flags.q.stream = 1;
                     if (state->has_enhanced_layouts()) {
      if (!this->flags.q.explicit_xfb_buffer) {
      if (q.flags.q.xfb_buffer) {
      this->flags.q.xfb_buffer = 1;
      } else if (!this->flags.q.xfb_buffer && this->flags.q.out &&
            /* Assign global xfb_buffer value */
   this->flags.q.xfb_buffer = 1;
                  if (q.flags.q.explicit_xfb_stride) {
      this->flags.q.xfb_stride = 1;
   this->flags.q.explicit_xfb_stride = 1;
                  if (q.flags.q.vertices) {
      if (this->flags.q.vertices
      && !is_single_layout_merge && !is_multiple_layouts_merge) {
      } else {
      this->flags.q.vertices = 1;
                  if (q.flags.q.vertex_spacing) {
      r &= validate_vertex_spacing(loc, state, *this, q);
   this->flags.q.vertex_spacing = 1;
               if (q.flags.q.ordering) {
      r &= validate_ordering(loc, state, *this, q);
   this->flags.q.ordering = 1;
               if (q.flags.q.point_mode) {
      r &= validate_point_mode(*this, q);
   this->flags.q.point_mode = 1;
               if (q.flags.q.early_fragment_tests)
            if ((q.flags.i & ubo_mat_mask.flags.i) != 0)
         if ((q.flags.i & ubo_layout_mask.flags.i) != 0)
            for (int i = 0; i < 3; i++) {
      if (q.flags.q.local_size & (1 << i)) {
      if (this->local_size[i]
      && !is_single_layout_merge && !is_multiple_layouts_merge) {
      } else {
                        if (q.flags.q.local_size_variable)
            if (q.flags.q.bindless_sampler)
            if (q.flags.q.bindless_image)
            if (q.flags.q.bound_sampler)
            if (q.flags.q.bound_image)
            if (q.flags.q.derivative_group) {
      this->flags.q.derivative_group = true;
                        if (this->flags.q.in &&
      (this->flags.i & ~input_layout_mask.flags.i) != 0) {
   _mesa_glsl_error(loc, state, "invalid input layout qualifier used");
               if (q.flags.q.explicit_align)
            if (q.flags.q.explicit_location)
            if (q.flags.q.explicit_index)
         if (q.flags.q.explicit_component)
               if (q.flags.q.explicit_binding)
            if (q.flags.q.explicit_offset || q.flags.q.explicit_xfb_offset)
            if (q.precision != ast_precision_none)
            if (q.flags.q.explicit_image_format) {
      this->image_format = q.image_format;
               if (q.flags.q.bindless_sampler ||
      q.flags.q.bindless_image ||
   q.flags.q.bound_sampler ||
   q.flags.q.bound_image)
         if (state->EXT_gpu_shader4_enable &&
      state->stage == MESA_SHADER_FRAGMENT &&
   this->flags.q.varying && q.flags.q.out) {
   this->flags.q.varying = 0;
                  }
      bool
   ast_type_qualifier::validate_out_qualifier(YYLTYPE *loc,
         {
      bool r = true;
   ast_type_qualifier valid_out_mask;
            switch (state->stage) {
   case MESA_SHADER_GEOMETRY:
      if (this->flags.q.prim_type) {
      /* Make sure this is a valid output primitive type. */
   switch (this->prim_type) {
   case GL_POINTS:
   case GL_LINE_STRIP:
   case GL_TRIANGLE_STRIP:
         default:
      r = false;
   _mesa_glsl_error(loc, state, "invalid geometry shader output "
                        valid_out_mask.flags.q.stream = 1;
   valid_out_mask.flags.q.explicit_stream = 1;
   valid_out_mask.flags.q.explicit_xfb_buffer = 1;
   valid_out_mask.flags.q.xfb_buffer = 1;
   valid_out_mask.flags.q.explicit_xfb_stride = 1;
   valid_out_mask.flags.q.xfb_stride = 1;
   valid_out_mask.flags.q.max_vertices = 1;
   valid_out_mask.flags.q.prim_type = 1;
      case MESA_SHADER_TESS_CTRL:
      valid_out_mask.flags.q.vertices = 1;
   valid_out_mask.flags.q.explicit_xfb_buffer = 1;
   valid_out_mask.flags.q.xfb_buffer = 1;
   valid_out_mask.flags.q.explicit_xfb_stride = 1;
   valid_out_mask.flags.q.xfb_stride = 1;
      case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_VERTEX:
      valid_out_mask.flags.q.explicit_xfb_buffer = 1;
   valid_out_mask.flags.q.xfb_buffer = 1;
   valid_out_mask.flags.q.explicit_xfb_stride = 1;
   valid_out_mask.flags.q.xfb_stride = 1;
      case MESA_SHADER_FRAGMENT:
      valid_out_mask.flags.q.blend_support = 1;
      default:
      r = false;
   _mesa_glsl_error(loc, state,
                     /* Generate an error when invalid output layout qualifiers are used. */
   if ((this->flags.i & ~valid_out_mask.flags.i) != 0) {
      r = false;
                  }
      bool
   ast_type_qualifier::merge_into_out_qualifier(YYLTYPE *loc,
               {
      const bool r = state->out_qualifier->merge_qualifier(loc, state,
            switch (state->stage) {
   case MESA_SHADER_GEOMETRY:
      /* Allow future assignments of global out's stream id value */
   state->out_qualifier->flags.q.explicit_stream = 0;
      case MESA_SHADER_TESS_CTRL:
      node = new(state->linalloc) ast_tcs_output_layout(*loc);
      default:
                  /* Allow future assignments of global out's */
   state->out_qualifier->flags.q.explicit_xfb_buffer = 0;
               }
      bool
   ast_type_qualifier::validate_in_qualifier(YYLTYPE *loc,
         {
      bool r = true;
   ast_type_qualifier valid_in_mask;
            switch (state->stage) {
   case MESA_SHADER_TESS_EVAL:
      if (this->flags.q.prim_type) {
      /* Make sure this is a valid input primitive type. */
   switch (this->prim_type) {
   case GL_TRIANGLES:
   case GL_QUADS:
   case GL_ISOLINES:
         default:
      r = false;
   _mesa_glsl_error(loc, state,
                              valid_in_mask.flags.q.prim_type = 1;
   valid_in_mask.flags.q.vertex_spacing = 1;
   valid_in_mask.flags.q.ordering = 1;
   valid_in_mask.flags.q.point_mode = 1;
      case MESA_SHADER_GEOMETRY:
      if (this->flags.q.prim_type) {
      /* Make sure this is a valid input primitive type. */
   switch (this->prim_type) {
   case GL_POINTS:
   case GL_LINES:
   case GL_LINES_ADJACENCY:
   case GL_TRIANGLES:
   case GL_TRIANGLES_ADJACENCY:
         default:
      r = false;
   _mesa_glsl_error(loc, state,
                        valid_in_mask.flags.q.prim_type = 1;
   valid_in_mask.flags.q.invocations = 1;
      case MESA_SHADER_FRAGMENT:
      valid_in_mask.flags.q.early_fragment_tests = 1;
   valid_in_mask.flags.q.inner_coverage = 1;
   valid_in_mask.flags.q.post_depth_coverage = 1;
   valid_in_mask.flags.q.pixel_interlock_ordered = 1;
   valid_in_mask.flags.q.pixel_interlock_unordered = 1;
   valid_in_mask.flags.q.sample_interlock_ordered = 1;
   valid_in_mask.flags.q.sample_interlock_unordered = 1;
      case MESA_SHADER_COMPUTE:
      valid_in_mask.flags.q.local_size = 7;
   valid_in_mask.flags.q.local_size_variable = 1;
   valid_in_mask.flags.q.derivative_group = 1;
      default:
      r = false;
   _mesa_glsl_error(loc, state,
                           /* Generate an error when invalid input layout qualifiers are used. */
   if ((this->flags.i & ~valid_in_mask.flags.i) != 0) {
      r = false;
               /* The checks below are also performed when merging but we want to spit an
   * error against the default global input qualifier as soon as we can, with
   * the closest error location in the shader.
   */
   r &= validate_prim_type(loc, state, *state->in_qualifier, *this);
   r &= validate_vertex_spacing(loc, state, *state->in_qualifier, *this);
   r &= validate_ordering(loc, state, *state->in_qualifier, *this);
               }
      bool
   ast_type_qualifier::merge_into_in_qualifier(YYLTYPE *loc,
               {
      bool r = true;
            /* We create the gs_input_layout node before merging so, in the future, no
   * more repeated nodes will be created as we will have the flag set.
   */
   if (state->stage == MESA_SHADER_GEOMETRY
      && this->flags.q.prim_type && !state->in_qualifier->flags.q.prim_type) {
                        if (state->in_qualifier->flags.q.early_fragment_tests) {
      state->fs_early_fragment_tests = true;
               if (state->in_qualifier->flags.q.inner_coverage) {
      state->fs_inner_coverage = true;
               if (state->in_qualifier->flags.q.post_depth_coverage) {
      state->fs_post_depth_coverage = true;
               if (state->fs_inner_coverage && state->fs_post_depth_coverage) {
      _mesa_glsl_error(loc, state,
                           if (state->in_qualifier->flags.q.pixel_interlock_ordered) {
      state->fs_pixel_interlock_ordered = true;
               if (state->in_qualifier->flags.q.pixel_interlock_unordered) {
      state->fs_pixel_interlock_unordered = true;
               if (state->in_qualifier->flags.q.sample_interlock_ordered) {
      state->fs_sample_interlock_ordered = true;
               if (state->in_qualifier->flags.q.sample_interlock_unordered) {
      state->fs_sample_interlock_unordered = true;
               if (state->fs_pixel_interlock_ordered +
      state->fs_pixel_interlock_unordered +
   state->fs_sample_interlock_ordered +
   state->fs_sample_interlock_unordered > 1) {
   _mesa_glsl_error(loc, state,
                     if (state->in_qualifier->flags.q.derivative_group) {
      if (state->cs_derivative_group != DERIVATIVE_GROUP_NONE) {
      if (state->in_qualifier->derivative_group != DERIVATIVE_GROUP_NONE &&
      state->cs_derivative_group != state->in_qualifier->derivative_group) {
   _mesa_glsl_error(loc, state,
               } else {
                     /* We allow the creation of multiple cs_input_layout nodes. Coherence among
   * all existing nodes is checked later, when the AST node is transformed
   * into HIR.
   */
   if (state->in_qualifier->flags.q.local_size) {
      node = new(lin_ctx) ast_cs_input_layout(*loc,
         state->in_qualifier->flags.q.local_size = 0;
   for (int i = 0; i < 3; i++)
               if (state->in_qualifier->flags.q.local_size_variable) {
      state->cs_input_local_size_variable_specified = true;
                  }
      bool
   ast_type_qualifier::push_to_global(YYLTYPE *loc,
         {
      if (this->flags.q.xfb_stride) {
               unsigned buff_idx;
   if (process_qualifier_constant(state, loc, "xfb_buffer",
            if (state->out_qualifier->out_xfb_stride[buff_idx]) {
      state->out_qualifier->out_xfb_stride[buff_idx]->merge_qualifier(
      new(state->linalloc) ast_layout_expression(*loc,
   } else {
      state->out_qualifier->out_xfb_stride[buff_idx] =
      new(state->linalloc) ast_layout_expression(*loc,
                     }
      /**
   * Check if the current type qualifier has any illegal flags.
   *
   * If so, print an error message, followed by a list of illegal flags.
   *
   * \param message        The error message to print.
   * \param allowed_flags  A list of valid flags.
   */
   bool
   ast_type_qualifier::validate_flags(YYLTYPE *loc,
                     {
      ast_type_qualifier bad;
   bad.flags.i = this->flags.i & ~allowed_flags.flags.i;
   if (bad.flags.i == 0)
               #define Q(f) \
      if (bad.flags.q.f) \
      #define Q2(f, s) \
      if (bad.flags.q.f) \
            Q(invariant);
   Q(precise);
   Q(constant);
   Q(attribute);
   Q(varying);
   Q(in);
   Q(out);
   Q(centroid);
   Q(sample);
   Q(patch);
   Q(uniform);
   Q(buffer);
   Q(shared_storage);
   Q(smooth);
   Q(flat);
   Q(noperspective);
   Q(origin_upper_left);
   Q(pixel_center_integer);
   Q2(explicit_align, align);
   Q2(explicit_component, component);
   Q2(explicit_location, location);
   Q2(explicit_index, index);
   Q2(explicit_binding, binding);
   Q2(explicit_offset, offset);
   Q(depth_type);
   Q(std140);
   Q(std430);
   Q(shared);
   Q(packed);
   Q(column_major);
   Q(row_major);
   Q(prim_type);
   Q(max_vertices);
   Q(local_size);
   Q(local_size_variable);
   Q(early_fragment_tests);
   Q2(explicit_image_format, image_format);
   Q(coherent);
   Q2(_volatile, volatile);
   Q(restrict_flag);
   Q(read_only);
   Q(write_only);
   Q(invocations);
   Q(stream);
   Q(stream);
   Q2(explicit_xfb_offset, xfb_offset);
   Q2(xfb_buffer, xfb_buffer);
   Q2(explicit_xfb_buffer, xfb_buffer);
   Q2(xfb_stride, xfb_stride);
   Q2(explicit_xfb_stride, xfb_stride);
   Q(vertex_spacing);
   Q(ordering);
   Q(point_mode);
   Q(vertices);
   Q(subroutine);
   Q(blend_support);
   Q(inner_coverage);
   Q(bindless_sampler);
   Q(bindless_image);
   Q(bound_sampler);
   Q(bound_image);
   Q(post_depth_coverage);
   Q(pixel_interlock_ordered);
   Q(pixel_interlock_unordered);
   Q(sample_interlock_ordered);
   Q(sample_interlock_unordered);
         #undef Q
   #undef Q2
         _mesa_glsl_error(loc, state,
                                 }
      bool
   ast_layout_expression::process_qualifier_constant(struct _mesa_glsl_parse_state *state,
                     {
      int min_value = 0;
   bool first_pass = true;
            if (!can_be_zero)
            for (exec_node *node = layout_const_expressions.get_head_raw();
               exec_list dummy_instructions;
                     ir_constant *const const_int =
            if (const_int == NULL || !const_int->type->is_integer_32()) {
      YYLTYPE loc = const_expression->get_location();
   _mesa_glsl_error(&loc, state, "%s must be an integral constant "
                     if (const_int->value.i[0] < min_value) {
      YYLTYPE loc = const_expression->get_location();
   _mesa_glsl_error(&loc, state, "%s layout qualifier is invalid "
                           if (!first_pass && *value != const_int->value.u[0]) {
      YYLTYPE loc = const_expression->get_location();
   _mesa_glsl_error(&loc, state, "%s layout qualifier does not "
      "match previous declaration (%d vs %d)",
         } else {
      first_pass = false;
               /* If the location is const (and we've verified that
   * it is) then no instructions should have been emitted
   * when we converted it to HIR. If they were emitted,
   * then either the location isn't const after all, or
   * we are emitting unnecessary instructions.
   */
                  }
      bool
   process_qualifier_constant(struct _mesa_glsl_parse_state *state,
                           {
               if (const_expression == NULL) {
      *value = 0;
                        ir_constant *const const_int =
         if (const_int == NULL || !const_int->type->is_integer_32()) {
      _mesa_glsl_error(loc, state, "%s must be an integral constant "
                     if (const_int->value.i[0] < 0) {
      _mesa_glsl_error(loc, state, "%s layout qualifier is invalid (%d < 0)",
                     /* If the location is const (and we've verified that
   * it is) then no instructions should have been emitted
   * when we converted it to HIR. If they were emitted,
   * then either the location isn't const after all, or
   * we are emitting unnecessary instructions.
   */
            *value = const_int->value.u[0];
      }
