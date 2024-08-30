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
   */
      /*
   * This lowering pass converts references to input/output variables with
   * loads/stores to actual input/output intrinsics.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
   #include "nir_xfb_info.h"
      #include "util/u_math.h"
      struct lower_io_state {
      void *dead_ctx;
   nir_builder builder;
   int (*type_size)(const struct glsl_type *type, bool);
   nir_variable_mode modes;
      };
      static nir_intrinsic_op
   ssbo_atomic_for_deref(nir_intrinsic_op deref_op)
   {
      switch (deref_op) {
   case nir_intrinsic_deref_atomic:
         case nir_intrinsic_deref_atomic_swap:
         default:
            }
      static nir_intrinsic_op
   global_atomic_for_deref(nir_address_format addr_format,
         {
      switch (deref_op) {
   case nir_intrinsic_deref_atomic:
      if (addr_format != nir_address_format_2x32bit_global)
         else
         case nir_intrinsic_deref_atomic_swap:
      if (addr_format != nir_address_format_2x32bit_global)
         else
         default:
            }
      static nir_intrinsic_op
   shared_atomic_for_deref(nir_intrinsic_op deref_op)
   {
      switch (deref_op) {
   case nir_intrinsic_deref_atomic:
         case nir_intrinsic_deref_atomic_swap:
         default:
            }
      static nir_intrinsic_op
   task_payload_atomic_for_deref(nir_intrinsic_op deref_op)
   {
      switch (deref_op) {
   case nir_intrinsic_deref_atomic:
         case nir_intrinsic_deref_atomic_swap:
         default:
            }
      void
   nir_assign_var_locations(nir_shader *shader, nir_variable_mode mode,
               {
               nir_foreach_variable_with_modes(var, shader, mode) {
      var->data.driver_location = location;
   bool bindless_type_size = var->data.mode == nir_var_shader_in ||
                              }
      /**
   * Some inputs and outputs are arrayed, meaning that there is an extra level
   * of array indexing to handle mismatches between the shader interface and the
   * dispatch pattern of the shader.  For instance, geometry shaders are
   * executed per-primitive while their inputs and outputs are specified
   * per-vertex so all inputs and outputs have to be additionally indexed with
   * the vertex index within the primitive.
   */
   bool
   nir_is_arrayed_io(const nir_variable *var, gl_shader_stage stage)
   {
      if (var->data.patch || !glsl_type_is_array(var->type))
            if (stage == MESA_SHADER_MESH) {
      /* NV_mesh_shader: this is flat array for the whole workgroup. */
   if (var->data.location == VARYING_SLOT_PRIMITIVE_INDICES)
               if (var->data.mode == nir_var_shader_in) {
      if (var->data.per_vertex) {
      assert(stage == MESA_SHADER_FRAGMENT);
               return stage == MESA_SHADER_GEOMETRY ||
                     if (var->data.mode == nir_var_shader_out)
      return stage == MESA_SHADER_TESS_CTRL ||
            }
      static bool
   uses_high_dvec2_semantic(struct lower_io_state *state,
         {
      return state->builder.shader->info.stage == MESA_SHADER_VERTEX &&
         state->options & nir_lower_io_lower_64bit_to_32_new &&
      }
      static unsigned
   get_number_of_slots(struct lower_io_state *state,
         {
               if (nir_is_arrayed_io(var, state->builder.shader->info.stage)) {
      assert(glsl_type_is_array(type));
               /* NV_mesh_shader:
   * PRIMITIVE_INDICES is a flat array, not a proper arrayed output,
   * as opposed to D3D-style mesh shaders where it's addressed by
   * the primitive index.
   * Prevent assigning several slots to primitive indices,
   * to avoid some issues.
   */
   if (state->builder.shader->info.stage == MESA_SHADER_MESH &&
      var->data.location == VARYING_SLOT_PRIMITIVE_INDICES &&
   !nir_is_arrayed_io(var, state->builder.shader->info.stage))
         return state->type_size(type, var->data.bindless) /
      }
      static nir_def *
   get_io_offset(nir_builder *b, nir_deref_instr *deref,
               nir_def **array_index,
   {
      nir_deref_path path;
            assert(path.path[0]->deref_type == nir_deref_type_var);
            /* For arrayed I/O (e.g., per-vertex input arrays in geometry shader
   * inputs), skip the outermost array index.  Process the rest normally.
   */
   if (array_index != NULL) {
      assert((*p)->deref_type == nir_deref_type_array);
   *array_index = (*p)->arr.index.ssa;
               if (path.path[0]->var->data.compact && nir_src_is_const((*p)->arr.index)) {
      assert((*p)->deref_type == nir_deref_type_array);
            /* We always lower indirect dereferences for "compact" array vars. */
   const unsigned index = nir_src_as_uint((*p)->arr.index);
   const unsigned total_offset = *component + index;
   const unsigned slot_offset = total_offset / 4;
   *component = total_offset % 4;
               /* Just emit code and let constant-folding go to town */
            for (; *p; p++) {
      if ((*p)->deref_type == nir_deref_type_array) {
                                 } else if ((*p)->deref_type == nir_deref_type_struct) {
                     unsigned field_offset = 0;
   for (unsigned i = 0; i < (*p)->strct.index; i++) {
         }
      } else {
                                 }
      static nir_def *
   emit_load(struct lower_io_state *state,
            nir_def *array_index, nir_variable *var, nir_def *offset,
      {
      nir_builder *b = &state->builder;
   const nir_shader *nir = b->shader;
   nir_variable_mode mode = var->data.mode;
            nir_intrinsic_op op;
   switch (mode) {
   case nir_var_shader_in:
      if (nir->info.stage == MESA_SHADER_FRAGMENT &&
      nir->options->use_interpolated_input_intrinsics &&
   var->data.interpolation != INTERP_MODE_FLAT &&
   !var->data.per_primitive) {
   if (var->data.interpolation == INTERP_MODE_EXPLICIT ||
      var->data.per_vertex) {
   assert(array_index != NULL);
                        nir_intrinsic_op bary_op;
   if (var->data.sample)
         else if (var->data.centroid)
                        barycentric = nir_load_barycentric(&state->builder, bary_op,
               } else {
         }
      case nir_var_shader_out:
      op = !array_index ? nir_intrinsic_load_output : var->data.per_primitive ? nir_intrinsic_load_per_primitive_output
            case nir_var_uniform:
      op = nir_intrinsic_load_uniform;
      default:
                  nir_intrinsic_instr *load =
                  nir_intrinsic_set_base(load, var->data.driver_location);
   if (nir_intrinsic_has_range(load)) {
      const struct glsl_type *type = var->type;
   if (array_index)
         unsigned var_size = state->type_size(type, var->data.bindless);
               if (mode == nir_var_shader_in || mode == nir_var_shader_out)
            if (nir_intrinsic_has_access(load))
                     if (load->intrinsic != nir_intrinsic_load_uniform) {
      nir_io_semantics semantics = { 0 };
   semantics.location = var->data.location;
   semantics.num_slots = get_number_of_slots(state, var);
   semantics.fb_fetch_output = var->data.fb_fetch_output;
   semantics.medium_precision =
      var->data.precision == GLSL_PRECISION_MEDIUM ||
      semantics.high_dvec2 = high_dvec2;
               if (array_index) {
      load->src[0] = nir_src_for_ssa(array_index);
      } else if (barycentric) {
      load->src[0] = nir_src_for_ssa(barycentric);
      } else {
                  nir_def_init(&load->instr, &load->def, num_components, bit_size);
               }
      static nir_def *
   lower_load(nir_intrinsic_instr *intrin, struct lower_io_state *state,
               {
      const bool lower_double = !glsl_type_is_integer(type) && state->options & nir_lower_io_lower_64bit_float_to_32;
   if (intrin->def.bit_size == 64 &&
      (lower_double || (state->options & (nir_lower_io_lower_64bit_to_32_new |
         nir_builder *b = &state->builder;
            /* Each slot is a dual slot, so divide the offset within the variable
   * by 2.
   */
   if (use_high_dvec2_semantic)
                     nir_def *comp64[4];
   assert(component == 0 || component == 2);
   unsigned dest_comp = 0;
   bool high_dvec2 = false;
   while (dest_comp < intrin->def.num_components) {
      const unsigned num_comps =
                  nir_def *data32 =
      emit_load(state, array_index, var, offset, component,
      for (unsigned i = 0; i < num_comps; i++) {
      comp64[dest_comp + i] =
               /* Only the first store has a component offset */
                  if (use_high_dvec2_semantic) {
      /* Increment the offset when we wrap around the dual slot. */
   if (high_dvec2)
            } else {
                        } else if (intrin->def.bit_size == 1) {
      /* Booleans are 32-bit */
   assert(glsl_type_is_boolean(type));
   return nir_b2b1(&state->builder,
                  } else {
      return emit_load(state, array_index, var, offset, component,
                     }
      static void
   emit_store(struct lower_io_state *state, nir_def *data,
            nir_def *array_index, nir_variable *var, nir_def *offset,
      {
               assert(var->data.mode == nir_var_shader_out);
   nir_intrinsic_op op =
      !array_index ? nir_intrinsic_store_output : var->data.per_primitive ? nir_intrinsic_store_per_primitive_output
         nir_intrinsic_instr *store =
                           const struct glsl_type *type = var->type;
   if (array_index)
         unsigned var_size = state->type_size(type, var->data.bindless);
   nir_intrinsic_set_base(store, var->data.driver_location);
   nir_intrinsic_set_range(store, var_size);
   nir_intrinsic_set_component(store, component);
                     if (nir_intrinsic_has_access(store))
            if (array_index)
                     unsigned gs_streams = 0;
   if (state->builder.shader->info.stage == MESA_SHADER_GEOMETRY) {
      if (var->data.stream & NIR_STREAM_PACKED) {
         } else {
      assert(var->data.stream < 4);
   gs_streams = 0;
   for (unsigned i = 0; i < num_components; ++i)
                  nir_io_semantics semantics = { 0 };
   semantics.location = var->data.location;
   semantics.num_slots = get_number_of_slots(state, var);
   semantics.dual_source_blend_index = var->data.index;
   semantics.gs_streams = gs_streams;
   semantics.medium_precision =
      var->data.precision == GLSL_PRECISION_MEDIUM ||
      semantics.per_view = var->data.per_view;
                        }
      static void
   lower_store(nir_intrinsic_instr *intrin, struct lower_io_state *state,
               {
      const bool lower_double = !glsl_type_is_integer(type) && state->options & nir_lower_io_lower_64bit_float_to_32;
   if (intrin->src[1].ssa->bit_size == 64 &&
      (lower_double || (state->options & (nir_lower_io_lower_64bit_to_32 |
                           assert(component == 0 || component == 2);
   unsigned src_comp = 0;
   nir_component_mask_t write_mask = nir_intrinsic_write_mask(intrin);
   while (src_comp < intrin->num_components) {
      const unsigned num_comps =
                  if (write_mask & BITFIELD_MASK(num_comps)) {
      nir_def *data =
                        nir_component_mask_t write_mask32 = 0;
   for (unsigned i = 0; i < num_comps; i++) {
                        emit_store(state, data32, array_index, var, offset,
                     /* Only the first store has a component offset */
   component = 0;
   src_comp += num_comps;
   write_mask >>= num_comps;
         } else if (intrin->def.bit_size == 1) {
      /* Booleans are 32-bit */
   assert(glsl_type_is_boolean(type));
   nir_def *b32_val = nir_b2b32(&state->builder, intrin->src[1].ssa);
   emit_store(state, b32_val, array_index, var, offset,
            component, intrin->num_components,
   } else {
      emit_store(state, intrin->src[1].ssa, array_index, var, offset,
            component, intrin->num_components,
      }
      static nir_def *
   lower_interpolate_at(nir_intrinsic_instr *intrin, struct lower_io_state *state,
               {
      nir_builder *b = &state->builder;
            /* Ignore interpolateAt() for flat variables - flat is flat. Lower
   * interpolateAtVertex() for explicit variables.
   */
   if (var->data.interpolation == INTERP_MODE_FLAT ||
      var->data.interpolation == INTERP_MODE_EXPLICIT) {
            if (var->data.interpolation == INTERP_MODE_EXPLICIT) {
      assert(intrin->intrinsic == nir_intrinsic_interp_deref_at_vertex);
                           /* None of the supported APIs allow interpolation on 64-bit things */
            nir_intrinsic_op bary_op;
   switch (intrin->intrinsic) {
   case nir_intrinsic_interp_deref_at_centroid:
      bary_op = nir_intrinsic_load_barycentric_centroid;
      case nir_intrinsic_interp_deref_at_sample:
      bary_op = nir_intrinsic_load_barycentric_at_sample;
      case nir_intrinsic_interp_deref_at_offset:
      bary_op = nir_intrinsic_load_barycentric_at_offset;
      default:
                  nir_intrinsic_instr *bary_setup =
            nir_def_init(&bary_setup->instr, &bary_setup->def, 2, 32);
            if (intrin->intrinsic == nir_intrinsic_interp_deref_at_sample ||
      intrin->intrinsic == nir_intrinsic_interp_deref_at_offset ||
   intrin->intrinsic == nir_intrinsic_interp_deref_at_vertex)
                  nir_io_semantics semantics = { 0 };
   semantics.location = var->data.location;
   semantics.num_slots = get_number_of_slots(state, var);
   semantics.medium_precision =
      var->data.precision == GLSL_PRECISION_MEDIUM ||
         nir_def *load =
      nir_load_interpolated_input(&state->builder,
                              intrin->def.num_components,
   intrin->def.bit_size,
               }
      static bool
   nir_lower_io_block(nir_block *block,
         {
      nir_builder *b = &state->builder;
   const nir_shader_compiler_options *options = b->shader->options;
            nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
      /* We can lower the io for this nir instrinsic */
      case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_vertex:
      /* We can optionally lower these to load_interpolated_input */
   if (options->use_interpolated_input_intrinsics ||
      options->lower_interpolate_at)
         default:
      /* We can't lower the io for this nir instrinsic, so skip it */
               nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (!nir_deref_mode_is_one_of(deref, state->modes))
                                       nir_def *offset;
   nir_def *array_index = NULL;
   unsigned component_offset = var->data.location_frac;
   bool bindless_type_size = var->data.mode == nir_var_shader_in ||
                  if (nir_deref_instr_is_known_out_of_bounds(deref)) {
      /* Section 5.11 (Out-of-Bounds Accesses) of the GLSL 4.60 spec says:
   *
   *    In the subsections described above for array, vector, matrix and
   *    structure accesses, any out-of-bounds access produced undefined
   *    behavior....
   *    Out-of-bounds reads return undefined values, which
   *    include values from other variables of the active program or zero.
   *    Out-of-bounds writes may be discarded or overwrite
   *    other variables of the active program.
   *
   * GL_KHR_robustness and GL_ARB_robustness encourage us to return zero
   * for reads.
   *
   * Otherwise get_io_offset would return out-of-bound offset which may
   * result in out-of-bound loading/storing of inputs/outputs,
   * that could cause issues in drivers down the line.
   */
   if (intrin->intrinsic != nir_intrinsic_store_deref) {
      nir_def *zero =
      nir_imm_zero(b, intrin->def.num_components,
      nir_def_rewrite_uses(&intrin->def,
               nir_instr_remove(&intrin->instr);
   progress = true;
               offset = get_io_offset(b, deref, is_arrayed ? &array_index : NULL,
                           switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
      replacement = lower_load(intrin, state, array_index, var, offset,
               case nir_intrinsic_store_deref:
      lower_store(intrin, state, array_index, var, offset,
               case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_vertex:
      assert(array_index == NULL);
   replacement = lower_interpolate_at(intrin, state, var, offset,
               default:
                  if (replacement) {
      nir_def_rewrite_uses(&intrin->def,
      }
   nir_instr_remove(&intrin->instr);
                  }
      static bool
   nir_lower_io_impl(nir_function_impl *impl,
                     {
      struct lower_io_state state;
            state.builder = nir_builder_create(impl);
   state.dead_ctx = ralloc_context(NULL);
   state.modes = modes;
   state.type_size = type_size;
            ASSERTED nir_variable_mode supported_modes =
                  nir_foreach_block(block, impl) {
                                       }
      /** Lower load/store_deref intrinsics on I/O variables to offset-based intrinsics
   *
   * This pass is intended to be used for cross-stage shader I/O and driver-
   * managed uniforms to turn deref-based access into a simpler model using
   * locations or offsets.  For fragment shader inputs, it can optionally turn
   * load_deref into an explicit interpolation using barycentrics coming from
   * one of the load_barycentric_* intrinsics.  This pass requires that all
   * deref chains are complete and contain no casts.
   */
   bool
   nir_lower_io(nir_shader *shader, nir_variable_mode modes,
               {
               nir_foreach_function_impl(impl, shader) {
                     }
      static unsigned
   type_scalar_size_bytes(const struct glsl_type *type)
   {
      assert(glsl_type_is_vector_or_scalar(type) ||
            }
      nir_def *
   nir_build_addr_iadd(nir_builder *b, nir_def *addr,
                     {
               switch (addr_format) {
   case nir_address_format_32bit_global:
   case nir_address_format_64bit_global:
   case nir_address_format_32bit_offset:
      assert(addr->bit_size == offset->bit_size);
   assert(addr->num_components == 1);
         case nir_address_format_2x32bit_global: {
      assert(addr->num_components == 2);
   nir_def *lo = nir_channel(b, addr, 0);
   nir_def *hi = nir_channel(b, addr, 1);
   nir_def *res_lo = nir_iadd(b, lo, offset);
   nir_def *carry = nir_b2i32(b, nir_ult(b, res_lo, lo));
   nir_def *res_hi = nir_iadd(b, hi, carry);
               case nir_address_format_32bit_offset_as_64bit:
      assert(addr->num_components == 1);
   assert(offset->bit_size == 32);
         case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      assert(addr->num_components == 4);
   assert(addr->bit_size == offset->bit_size);
         case nir_address_format_32bit_index_offset:
      assert(addr->num_components == 2);
   assert(addr->bit_size == offset->bit_size);
         case nir_address_format_32bit_index_offset_pack64:
      assert(addr->num_components == 1);
   assert(offset->bit_size == 32);
   return nir_pack_64_2x32_split(b,
               case nir_address_format_vec2_index_32bit_offset:
      assert(addr->num_components == 3);
   assert(offset->bit_size == 32);
         case nir_address_format_62bit_generic:
      assert(addr->num_components == 1);
   assert(addr->bit_size == 64);
   assert(offset->bit_size == 64);
   if (!(modes & ~(nir_var_function_temp |
                  /* If we're sure it's one of these modes, we can do an easy 32-bit
   * addition and don't need to bother with 64-bit math.
   */
   nir_def *addr32 = nir_unpack_64_2x32_split_x(b, addr);
   nir_def *type = nir_unpack_64_2x32_split_y(b, addr);
   addr32 = nir_iadd(b, addr32, nir_u2u32(b, offset));
      } else {
               case nir_address_format_logical:
         }
      }
      static unsigned
   addr_get_offset_bit_size(nir_def *addr, nir_address_format addr_format)
   {
      if (addr_format == nir_address_format_32bit_offset_as_64bit ||
      addr_format == nir_address_format_32bit_index_offset_pack64)
         }
      nir_def *
   nir_build_addr_iadd_imm(nir_builder *b, nir_def *addr,
                     {
      if (!offset)
            return nir_build_addr_iadd(
      b, addr, addr_format, modes,
   nir_imm_intN_t(b, offset,
   }
      static nir_def *
   build_addr_for_var(nir_builder *b, nir_variable *var,
         {
      assert(var->data.mode & (nir_var_uniform | nir_var_mem_shared |
                              const unsigned num_comps = nir_address_format_num_components(addr_format);
            switch (addr_format) {
   case nir_address_format_2x32bit_global:
   case nir_address_format_32bit_global:
   case nir_address_format_64bit_global: {
      nir_def *base_addr;
   switch (var->data.mode) {
   case nir_var_shader_temp:
                  case nir_var_function_temp:
                  case nir_var_mem_constant:
                  case nir_var_mem_shared:
                  case nir_var_mem_global:
                  default:
                  return nir_build_addr_iadd_imm(b, base_addr, addr_format, var->data.mode,
               case nir_address_format_32bit_offset:
      assert(var->data.driver_location <= UINT32_MAX);
         case nir_address_format_32bit_offset_as_64bit:
      assert(var->data.driver_location <= UINT32_MAX);
         case nir_address_format_62bit_generic:
      switch (var->data.mode) {
   case nir_var_shader_temp:
   case nir_var_function_temp:
                  case nir_var_mem_shared:
                  case nir_var_mem_global:
                  default:
               default:
            }
      static nir_def *
   build_runtime_addr_mode_check(nir_builder *b, nir_def *addr,
               {
      /* The compile-time check failed; do a run-time check */
   switch (addr_format) {
   case nir_address_format_62bit_generic: {
      assert(addr->num_components == 1);
   assert(addr->bit_size == 64);
   nir_def *mode_enum = nir_ushr_imm(b, addr, 62);
   switch (mode) {
   case nir_var_function_temp:
   case nir_var_shader_temp:
            case nir_var_mem_shared:
            case nir_var_mem_global:
                  default:
                     default:
            }
      unsigned
   nir_address_format_bit_size(nir_address_format addr_format)
   {
      switch (addr_format) {
   case nir_address_format_32bit_global:
         case nir_address_format_2x32bit_global:
         case nir_address_format_64bit_global:
         case nir_address_format_64bit_global_32bit_offset:
         case nir_address_format_64bit_bounded_global:
         case nir_address_format_32bit_index_offset:
         case nir_address_format_32bit_index_offset_pack64:
         case nir_address_format_vec2_index_32bit_offset:
         case nir_address_format_62bit_generic:
         case nir_address_format_32bit_offset:
         case nir_address_format_32bit_offset_as_64bit:
         case nir_address_format_logical:
         }
      }
      unsigned
   nir_address_format_num_components(nir_address_format addr_format)
   {
      switch (addr_format) {
   case nir_address_format_32bit_global:
         case nir_address_format_2x32bit_global:
         case nir_address_format_64bit_global:
         case nir_address_format_64bit_global_32bit_offset:
         case nir_address_format_64bit_bounded_global:
         case nir_address_format_32bit_index_offset:
         case nir_address_format_32bit_index_offset_pack64:
         case nir_address_format_vec2_index_32bit_offset:
         case nir_address_format_62bit_generic:
         case nir_address_format_32bit_offset:
         case nir_address_format_32bit_offset_as_64bit:
         case nir_address_format_logical:
         }
      }
      static nir_def *
   addr_to_index(nir_builder *b, nir_def *addr,
         {
      switch (addr_format) {
   case nir_address_format_32bit_index_offset:
      assert(addr->num_components == 2);
      case nir_address_format_32bit_index_offset_pack64:
         case nir_address_format_vec2_index_32bit_offset:
      assert(addr->num_components == 3);
      default:
            }
      static nir_def *
   addr_to_offset(nir_builder *b, nir_def *addr,
         {
      switch (addr_format) {
   case nir_address_format_32bit_index_offset:
      assert(addr->num_components == 2);
      case nir_address_format_32bit_index_offset_pack64:
         case nir_address_format_vec2_index_32bit_offset:
      assert(addr->num_components == 3);
      case nir_address_format_32bit_offset:
         case nir_address_format_32bit_offset_as_64bit:
   case nir_address_format_62bit_generic:
         default:
            }
      /** Returns true if the given address format resolves to a global address */
   static bool
   addr_format_is_global(nir_address_format addr_format,
         {
      if (addr_format == nir_address_format_62bit_generic)
            return addr_format == nir_address_format_32bit_global ||
         addr_format == nir_address_format_2x32bit_global ||
   addr_format == nir_address_format_64bit_global ||
      }
      static bool
   addr_format_is_offset(nir_address_format addr_format,
         {
      if (addr_format == nir_address_format_62bit_generic)
            return addr_format == nir_address_format_32bit_offset ||
      }
      static nir_def *
   addr_to_global(nir_builder *b, nir_def *addr,
         {
      switch (addr_format) {
   case nir_address_format_32bit_global:
   case nir_address_format_64bit_global:
   case nir_address_format_62bit_generic:
      assert(addr->num_components == 1);
         case nir_address_format_2x32bit_global:
      assert(addr->num_components == 2);
         case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      assert(addr->num_components == 4);
   return nir_iadd(b, nir_pack_64_2x32(b, nir_trim_vector(b, addr, 2)),
         case nir_address_format_32bit_index_offset:
   case nir_address_format_32bit_index_offset_pack64:
   case nir_address_format_vec2_index_32bit_offset:
   case nir_address_format_32bit_offset:
   case nir_address_format_32bit_offset_as_64bit:
   case nir_address_format_logical:
                     }
      static bool
   addr_format_needs_bounds_check(nir_address_format addr_format)
   {
         }
      static nir_def *
   addr_is_in_bounds(nir_builder *b, nir_def *addr,
         {
      assert(addr_format == nir_address_format_64bit_bounded_global);
   assert(addr->num_components == 4);
   assert(size > 0);
   return nir_ult(b, nir_iadd_imm(b, nir_channel(b, addr, 3), size - 1),
      }
      static void
   nir_get_explicit_deref_range(nir_deref_instr *deref,
                     {
      uint32_t base = 0;
            while (true) {
               switch (deref->deref_type) {
   case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
   case nir_deref_type_ptr_as_array: {
      const unsigned stride = nir_deref_instr_array_stride(deref);
                                 if (deref->deref_type != nir_deref_type_array_wildcard &&
      nir_src_is_const(deref->arr.index)) {
      } else {
      if (glsl_get_length(parent->type) == 0)
            }
               case nir_deref_type_struct: {
                     base += glsl_get_struct_field_offset(parent->type, deref->strct.index);
               case nir_deref_type_cast: {
               switch (parent_instr->type) {
                     switch (addr_format) {
   case nir_address_format_32bit_offset:
      base += load->value[1].u32;
      case nir_address_format_32bit_index_offset:
      base += load->value[1].u32;
      case nir_address_format_vec2_index_32bit_offset:
      base += load->value[2].u32;
      default:
                  *out_base = base;
   *out_range = range;
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(parent_instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_load_vulkan_descriptor:
      /* Assume that a load_vulkan_descriptor won't contribute to an
   * offset within the resource.
   */
      default:
                  *out_base = base;
   *out_range = range;
               default:
                     default:
                           fail:
      *out_base = 0;
      }
      static nir_variable_mode
   canonicalize_generic_modes(nir_variable_mode modes)
   {
      assert(modes != 0);
   if (util_bitcount(modes) == 1)
            assert(!(modes & ~(nir_var_function_temp | nir_var_shader_temp |
            /* Canonicalize by converting shader_temp to function_temp */
   if (modes & nir_var_shader_temp) {
      modes &= ~nir_var_shader_temp;
                  }
      static nir_intrinsic_op
   get_store_global_op_from_addr_format(nir_address_format addr_format)
   {
      if (addr_format != nir_address_format_2x32bit_global)
         else
      }
      static nir_intrinsic_op
   get_load_global_op_from_addr_format(nir_address_format addr_format)
   {
      if (addr_format != nir_address_format_2x32bit_global)
         else
      }
      static nir_intrinsic_op
   get_load_global_constant_op_from_addr_format(nir_address_format addr_format)
   {
      if (addr_format != nir_address_format_2x32bit_global)
         else
      }
      static nir_def *
   build_explicit_io_load(nir_builder *b, nir_intrinsic_instr *intrin,
                           {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            if (util_bitcount(modes) > 1) {
      if (addr_format_is_global(addr_format, modes)) {
      return build_explicit_io_load(b, intrin, addr, addr_format,
                  } else if (modes & nir_var_function_temp) {
      nir_push_if(b, build_runtime_addr_mode_check(b, addr, addr_format,
         nir_def *res1 =
      build_explicit_io_load(b, intrin, addr, addr_format,
                  nir_push_else(b, NULL);
   nir_def *res2 =
      build_explicit_io_load(b, intrin, addr, addr_format,
                  nir_pop_if(b, NULL);
      } else {
      nir_push_if(b, build_runtime_addr_mode_check(b, addr, addr_format,
         assert(modes & nir_var_mem_shared);
   nir_def *res1 =
      build_explicit_io_load(b, intrin, addr, addr_format,
                  nir_push_else(b, NULL);
   assert(modes & nir_var_mem_global);
   nir_def *res2 =
      build_explicit_io_load(b, intrin, addr, addr_format,
                  nir_pop_if(b, NULL);
                  assert(util_bitcount(modes) == 1);
            nir_intrinsic_op op;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
      switch (mode) {
   case nir_var_mem_ubo:
      if (addr_format == nir_address_format_64bit_global_32bit_offset)
         else if (addr_format == nir_address_format_64bit_bounded_global)
         else if (addr_format_is_global(addr_format, mode))
         else
            case nir_var_mem_ssbo:
      if (addr_format_is_global(addr_format, mode))
         else
            case nir_var_mem_global:
      assert(addr_format_is_global(addr_format, mode));
   op = get_load_global_op_from_addr_format(addr_format);
      case nir_var_uniform:
      assert(addr_format_is_offset(addr_format, mode));
   assert(b->shader->info.stage == MESA_SHADER_KERNEL);
   op = nir_intrinsic_load_kernel_input;
      case nir_var_mem_shared:
      assert(addr_format_is_offset(addr_format, mode));
   op = nir_intrinsic_load_shared;
      case nir_var_mem_task_payload:
      assert(addr_format_is_offset(addr_format, mode));
   op = nir_intrinsic_load_task_payload;
      case nir_var_shader_temp:
   case nir_var_function_temp:
      if (addr_format_is_offset(addr_format, mode)) {
         } else {
      assert(addr_format_is_global(addr_format, mode));
      }
      case nir_var_mem_push_const:
      assert(addr_format == nir_address_format_32bit_offset);
   op = nir_intrinsic_load_push_constant;
      case nir_var_mem_constant:
      if (addr_format_is_offset(addr_format, mode)) {
         } else {
      assert(addr_format_is_global(addr_format, mode));
      }
      default:
         }
         case nir_intrinsic_load_deref_block_intel:
      switch (mode) {
   case nir_var_mem_ssbo:
      if (addr_format_is_global(addr_format, mode))
         else
            case nir_var_mem_global:
      op = nir_intrinsic_load_global_block_intel;
      case nir_var_mem_shared:
      op = nir_intrinsic_load_shared_block_intel;
      default:
         }
         default:
                           if (op == nir_intrinsic_load_global_constant_offset) {
      assert(addr_format == nir_address_format_64bit_global_32bit_offset);
   load->src[0] = nir_src_for_ssa(
            } else if (op == nir_intrinsic_load_global_constant_bounded) {
      assert(addr_format == nir_address_format_64bit_bounded_global);
   load->src[0] = nir_src_for_ssa(
         load->src[1] = nir_src_for_ssa(nir_channel(b, addr, 3));
      } else if (addr_format_is_global(addr_format, mode)) {
         } else if (addr_format_is_offset(addr_format, mode)) {
      assert(addr->num_components == 1);
      } else {
      load->src[0] = nir_src_for_ssa(addr_to_index(b, addr, addr_format));
               if (nir_intrinsic_has_access(load))
            if (op == nir_intrinsic_load_constant) {
      nir_intrinsic_set_base(load, 0);
      } else if (op == nir_intrinsic_load_kernel_input) {
      nir_intrinsic_set_base(load, 0);
      } else if (mode == nir_var_mem_push_const) {
      /* Push constants are required to be able to be chased back to the
   * variable so we can provide a base/range.
   */
   nir_variable *var = nir_deref_instr_get_variable(deref);
   nir_intrinsic_set_base(load, 0);
               unsigned bit_size = intrin->def.bit_size;
   if (bit_size == 1) {
      /* TODO: Make the native bool bit_size an option. */
               if (nir_intrinsic_has_align(load))
            if (nir_intrinsic_has_range_base(load)) {
      unsigned base, range;
   nir_get_explicit_deref_range(deref, addr_format, &base, &range);
   nir_intrinsic_set_range_base(load, base);
               load->num_components = num_components;
                     nir_def *result;
   if (addr_format_needs_bounds_check(addr_format) &&
      op != nir_intrinsic_load_global_constant_bounded) {
   /* We don't need to bounds-check global_constant_bounded because bounds
   * checking is handled by the intrinsic itself.
   *
   * The Vulkan spec for robustBufferAccess gives us quite a few options
   * as to what we can do with an OOB read.  Unfortunately, returning
   * undefined values isn't one of them so we return an actual zero.
   */
            /* TODO: Better handle block_intel. */
   assert(load->num_components == 1);
   const unsigned load_size = bit_size / 8;
                                 } else {
      nir_builder_instr_insert(b, &load->instr);
               if (intrin->def.bit_size == 1) {
      /* For shared, we can go ahead and use NIR's and/or the back-end's
   * standard encoding for booleans rather than forcing a 0/1 boolean.
   * This should save an instruction or two.
   */
   if (mode == nir_var_mem_shared ||
      mode == nir_var_shader_temp ||
   mode == nir_var_function_temp)
      else
                  }
      static void
   build_explicit_io_store(nir_builder *b, nir_intrinsic_instr *intrin,
                           {
               if (util_bitcount(modes) > 1) {
      if (addr_format_is_global(addr_format, modes)) {
      build_explicit_io_store(b, intrin, addr, addr_format,
                  } else if (modes & nir_var_function_temp) {
      nir_push_if(b, build_runtime_addr_mode_check(b, addr, addr_format,
         build_explicit_io_store(b, intrin, addr, addr_format,
                     nir_push_else(b, NULL);
   build_explicit_io_store(b, intrin, addr, addr_format,
                        } else {
      nir_push_if(b, build_runtime_addr_mode_check(b, addr, addr_format,
         assert(modes & nir_var_mem_shared);
   build_explicit_io_store(b, intrin, addr, addr_format,
                     nir_push_else(b, NULL);
   assert(modes & nir_var_mem_global);
   build_explicit_io_store(b, intrin, addr, addr_format,
                        }
               assert(util_bitcount(modes) == 1);
            nir_intrinsic_op op;
   switch (intrin->intrinsic) {
   case nir_intrinsic_store_deref:
               switch (mode) {
   case nir_var_mem_ssbo:
      if (addr_format_is_global(addr_format, mode))
         else
            case nir_var_mem_global:
      assert(addr_format_is_global(addr_format, mode));
   op = get_store_global_op_from_addr_format(addr_format);
      case nir_var_mem_shared:
      assert(addr_format_is_offset(addr_format, mode));
   op = nir_intrinsic_store_shared;
      case nir_var_mem_task_payload:
      assert(addr_format_is_offset(addr_format, mode));
   op = nir_intrinsic_store_task_payload;
      case nir_var_shader_temp:
   case nir_var_function_temp:
      if (addr_format_is_offset(addr_format, mode)) {
         } else {
      assert(addr_format_is_global(addr_format, mode));
      }
      default:
         }
         case nir_intrinsic_store_deref_block_intel:
               switch (mode) {
   case nir_var_mem_ssbo:
      if (addr_format_is_global(addr_format, mode))
         else
            case nir_var_mem_global:
      op = nir_intrinsic_store_global_block_intel;
      case nir_var_mem_shared:
      op = nir_intrinsic_store_shared_block_intel;
      default:
         }
         default:
                           if (value->bit_size == 1) {
      /* For shared, we can go ahead and use NIR's and/or the back-end's
   * standard encoding for booleans rather than forcing a 0/1 boolean.
   * This should save an instruction or two.
   *
   * TODO: Make the native bool bit_size an option.
   */
   if (mode == nir_var_mem_shared ||
      mode == nir_var_shader_temp ||
   mode == nir_var_function_temp)
      else
               store->src[0] = nir_src_for_ssa(value);
   if (addr_format_is_global(addr_format, mode)) {
         } else if (addr_format_is_offset(addr_format, mode)) {
      assert(addr->num_components == 1);
      } else {
      store->src[1] = nir_src_for_ssa(addr_to_index(b, addr, addr_format));
                        if (nir_intrinsic_has_access(store))
                     assert(value->num_components == 1 ||
                           if (addr_format_needs_bounds_check(addr_format)) {
      /* TODO: Better handle block_intel. */
   assert(store->num_components == 1);
   const unsigned store_size = value->bit_size / 8;
                        } else {
            }
      static nir_def *
   build_explicit_io_atomic(nir_builder *b, nir_intrinsic_instr *intrin,
               {
               if (util_bitcount(modes) > 1) {
      if (addr_format_is_global(addr_format, modes)) {
      return build_explicit_io_atomic(b, intrin, addr, addr_format,
      } else if (modes & nir_var_function_temp) {
      nir_push_if(b, build_runtime_addr_mode_check(b, addr, addr_format,
         nir_def *res1 =
      build_explicit_io_atomic(b, intrin, addr, addr_format,
      nir_push_else(b, NULL);
   nir_def *res2 =
      build_explicit_io_atomic(b, intrin, addr, addr_format,
      nir_pop_if(b, NULL);
      } else {
      nir_push_if(b, build_runtime_addr_mode_check(b, addr, addr_format,
         assert(modes & nir_var_mem_shared);
   nir_def *res1 =
      build_explicit_io_atomic(b, intrin, addr, addr_format,
      nir_push_else(b, NULL);
   assert(modes & nir_var_mem_global);
   nir_def *res2 =
      build_explicit_io_atomic(b, intrin, addr, addr_format,
      nir_pop_if(b, NULL);
                  assert(util_bitcount(modes) == 1);
            const unsigned num_data_srcs =
            nir_intrinsic_op op;
   switch (mode) {
   case nir_var_mem_ssbo:
      if (addr_format_is_global(addr_format, mode))
         else
            case nir_var_mem_global:
      assert(addr_format_is_global(addr_format, mode));
   op = global_atomic_for_deref(addr_format, intrin->intrinsic);
      case nir_var_mem_shared:
      assert(addr_format_is_offset(addr_format, mode));
   op = shared_atomic_for_deref(intrin->intrinsic);
      case nir_var_mem_task_payload:
      assert(addr_format_is_offset(addr_format, mode));
   op = task_payload_atomic_for_deref(intrin->intrinsic);
      default:
                  nir_intrinsic_instr *atomic = nir_intrinsic_instr_create(b->shader, op);
            unsigned src = 0;
   if (addr_format_is_global(addr_format, mode)) {
         } else if (addr_format_is_offset(addr_format, mode)) {
      assert(addr->num_components == 1);
      } else {
      atomic->src[src++] = nir_src_for_ssa(addr_to_index(b, addr, addr_format));
      }
   for (unsigned i = 0; i < num_data_srcs; i++) {
                  /* Global atomics don't have access flags because they assume that the
   * address may be non-uniform.
   */
   if (nir_intrinsic_has_access(atomic))
            assert(intrin->def.num_components == 1);
   nir_def_init(&atomic->instr, &atomic->def, 1,
                     if (addr_format_needs_bounds_check(addr_format)) {
      const unsigned atomic_size = atomic->def.bit_size / 8;
                     nir_pop_if(b, NULL);
   return nir_if_phi(b, &atomic->def,
      } else {
      nir_builder_instr_insert(b, &atomic->instr);
         }
      nir_def *
   nir_explicit_io_address_from_deref(nir_builder *b, nir_deref_instr *deref,
               {
      switch (deref->deref_type) {
   case nir_deref_type_var:
            case nir_deref_type_ptr_as_array:
   case nir_deref_type_array: {
      unsigned stride = nir_deref_instr_array_stride(deref);
            unsigned offset_bit_size = addr_get_offset_bit_size(base_addr, addr_format);
   nir_def *index = deref->arr.index.ssa;
            /* If the access chain has been declared in-bounds, then we know it doesn't
   * overflow the type.  For nir_deref_type_array, this implies it cannot be
   * negative. Also, since types in NIR have a maximum 32-bit size, we know the
   * final result will fit in a 32-bit value so we can convert the index to
   * 32-bit before multiplying and save ourselves from a 64-bit multiply.
   */
   if (deref->arr.in_bounds && deref->deref_type == nir_deref_type_array) {
      index = nir_u2u32(b, index);
      } else {
      index = nir_i2iN(b, index, offset_bit_size);
               return nir_build_addr_iadd(b, base_addr, addr_format,
               case nir_deref_type_array_wildcard:
      unreachable("Wildcards should be lowered by now");
         case nir_deref_type_struct: {
      nir_deref_instr *parent = nir_deref_instr_parent(deref);
   int offset = glsl_get_struct_field_offset(parent->type,
         assert(offset >= 0);
   return nir_build_addr_iadd_imm(b, base_addr, addr_format,
               case nir_deref_type_cast:
      /* Nothing to do here */
                  }
      void
   nir_lower_explicit_io_instr(nir_builder *b,
                     {
               nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   unsigned vec_stride = glsl_get_explicit_stride(deref->type);
   unsigned scalar_size = type_scalar_size_bytes(deref->type);
   if (vec_stride == 0) {
         } else {
      assert(glsl_type_is_vector(deref->type));
               uint32_t align_mul, align_offset;
   if (!nir_get_explicit_deref_align(deref, true, &align_mul, &align_offset)) {
      /* If we don't have an alignment from the deref, assume scalar */
   align_mul = scalar_size;
               /* In order for bounds checking to be correct as per the Vulkan spec,
   * we need to check at the individual component granularity.  Prior to
   * robustness2, we're technically allowed to be sloppy by 16B.  Even with
   * robustness2, UBO loads are allowed to have a granularity as high as 256B
   * depending on hardware limits.  However, we have none of that information
   * here.  Short of adding new address formats, the easiest way to do that
   * is to just split any loads and stores into individual components here.
   *
   * TODO: At some point in the future we may want to add more ops similar to
   * nir_intrinsic_load_global_constant_bounded and make bouds checking the
   * back-end's problem.  Another option would be to somehow plumb more of
   * that information through to nir_lower_explicit_io.  For now, however,
   * scalarizing is at least correct.
   */
   bool scalarize = vec_stride > scalar_size ||
            switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref: {
      nir_def *value;
   if (scalarize) {
      nir_def *comps[NIR_MAX_VEC_COMPONENTS] = {
         };
   for (unsigned i = 0; i < intrin->num_components; i++) {
      unsigned comp_offset = i * vec_stride;
   nir_def *comp_addr = nir_build_addr_iadd_imm(b, addr, addr_format,
               comps[i] = build_explicit_io_load(b, intrin, comp_addr,
                              }
      } else {
      value = build_explicit_io_load(b, intrin, addr, addr_format,
            }
   nir_def_rewrite_uses(&intrin->def, value);
               case nir_intrinsic_store_deref: {
      nir_def *value = intrin->src[1].ssa;
   nir_component_mask_t write_mask = nir_intrinsic_write_mask(intrin);
   if (scalarize) {
      for (unsigned i = 0; i < intrin->num_components; i++) {
                     unsigned comp_offset = i * vec_stride;
   nir_def *comp_addr = nir_build_addr_iadd_imm(b, addr, addr_format,
               build_explicit_io_store(b, intrin, comp_addr, addr_format,
                     } else {
      build_explicit_io_store(b, intrin, addr, addr_format,
            }
               case nir_intrinsic_load_deref_block_intel: {
      nir_def *value = build_explicit_io_load(b, intrin, addr, addr_format,
                     nir_def_rewrite_uses(&intrin->def, value);
               case nir_intrinsic_store_deref_block_intel: {
      nir_def *value = intrin->src[1].ssa;
   const nir_component_mask_t write_mask = 0;
   build_explicit_io_store(b, intrin, addr, addr_format,
                           default: {
      nir_def *value =
         nir_def_rewrite_uses(&intrin->def, value);
      }
               }
      bool
   nir_get_explicit_deref_align(nir_deref_instr *deref,
                     {
      if (deref->deref_type == nir_deref_type_var) {
      /* If we see a variable, align_mul is effectively infinite because we
   * know the offset exactly (up to the offset of the base pointer for the
   * given variable mode).   We have to pick something so we choose 256B
   * as an arbitrary alignment which seems high enough for any reasonable
   * wide-load use-case.  Back-ends should clamp alignments down if 256B
   * is too large for some reason.
   */
   *align_mul = 256;
   *align_offset = deref->var->data.driver_location % 256;
               /* If we're a cast deref that has an alignment, use that. */
   if (deref->deref_type == nir_deref_type_cast && deref->cast.align_mul > 0) {
      *align_mul = deref->cast.align_mul;
   *align_offset = deref->cast.align_offset;
               /* Otherwise, we need to compute the alignment based on the parent */
   nir_deref_instr *parent = nir_deref_instr_parent(deref);
   if (parent == NULL) {
      assert(deref->deref_type == nir_deref_type_cast);
   if (default_to_type_align) {
      /* If we don't have a parent, assume the type's alignment, if any. */
   unsigned type_align = glsl_get_explicit_alignment(deref->type);
                  *align_mul = type_align;
   *align_offset = 0;
      } else {
                     uint32_t parent_mul, parent_offset;
   if (!nir_get_explicit_deref_align(parent, default_to_type_align,
                  switch (deref->deref_type) {
   case nir_deref_type_var:
            case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
   case nir_deref_type_ptr_as_array: {
      const unsigned stride = nir_deref_instr_array_stride(deref);
   if (stride == 0)
            if (deref->deref_type != nir_deref_type_array_wildcard &&
      nir_src_is_const(deref->arr.index)) {
   unsigned offset = nir_src_as_uint(deref->arr.index) * stride;
   *align_mul = parent_mul;
      } else {
      /* If this is a wildcard or an indirect deref, we have to go with the
   * power-of-two gcd.
   */
   *align_mul = MIN2(parent_mul, 1 << (ffs(stride) - 1));
      }
               case nir_deref_type_struct: {
      const int offset = glsl_get_struct_field_offset(parent->type,
         if (offset < 0)
            *align_mul = parent_mul;
   *align_offset = (parent_offset + offset) % parent_mul;
               case nir_deref_type_cast:
      /* We handled the explicit alignment case above. */
   assert(deref->cast.align_mul == 0);
   *align_mul = parent_mul;
   *align_offset = parent_offset;
                  }
      static void
   lower_explicit_io_deref(nir_builder *b, nir_deref_instr *deref,
         {
      /* Ignore samplers/textures, because they are handled by other passes like `nir_lower_samplers`.
   * Also do it only for those being uniforms, otherwise it will break GL bindless textures handles
   * stored in UBOs.
   */
   if (nir_deref_mode_is_in_set(deref, nir_var_uniform) &&
      (glsl_type_is_sampler(deref->type) ||
   glsl_type_is_texture(deref->type)))
         /* Just delete the deref if it's not used.  We can't use
   * nir_deref_instr_remove_if_unused here because it may remove more than
   * one deref which could break our list walking since we walk the list
   * backwards.
   */
   if (nir_def_is_unused(&deref->def)) {
      nir_instr_remove(&deref->instr);
                        nir_def *base_addr = NULL;
   if (deref->deref_type != nir_deref_type_var) {
                  nir_def *addr = nir_explicit_io_address_from_deref(b, deref, base_addr,
         assert(addr->bit_size == deref->def.bit_size);
            nir_instr_remove(&deref->instr);
      }
      static void
   lower_explicit_io_access(nir_builder *b, nir_intrinsic_instr *intrin,
         {
         }
      static void
   lower_explicit_io_array_length(nir_builder *b, nir_intrinsic_instr *intrin,
         {
                        assert(glsl_type_is_array(deref->type));
   assert(glsl_get_length(deref->type) == 0);
   assert(nir_deref_mode_is(deref, nir_var_mem_ssbo));
   unsigned stride = glsl_get_explicit_stride(deref->type);
                     nir_def *offset, *size;
   switch (addr_format) {
   case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      offset = nir_channel(b, addr, 3);
   size = nir_channel(b, addr, 2);
         case nir_address_format_32bit_index_offset:
   case nir_address_format_32bit_index_offset_pack64:
   case nir_address_format_vec2_index_32bit_offset: {
      offset = addr_to_offset(b, addr, addr_format);
   nir_def *index = addr_to_index(b, addr, addr_format);
   unsigned access = nir_intrinsic_access(intrin);
   size = nir_get_ssbo_size(b, index, .access = access);
               default:
                  nir_def *remaining = nir_usub_sat(b, size, offset);
            nir_def_rewrite_uses(&intrin->def, arr_size);
      }
      static void
   lower_explicit_io_mode_check(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (addr_format_is_global(addr_format, 0)) {
      /* If the address format is always global, then the driver can use
   * global addresses regardless of the mode.  In that case, don't create
   * a check, just whack the intrinsic to addr_mode_is and delegate to the
   * driver lowering.
   */
   intrin->intrinsic = nir_intrinsic_addr_mode_is;
                                 nir_def *is_mode =
      build_runtime_addr_mode_check(b, addr, addr_format,
            }
      static bool
   nir_lower_explicit_io_impl(nir_function_impl *impl, nir_variable_mode modes,
         {
                        /* Walk in reverse order so that we can see the full deref chain when we
   * lower the access operations.  We lower them assuming that the derefs
   * will be turned into address calculations later.
   */
   nir_foreach_block_reverse(block, impl) {
      nir_foreach_instr_reverse_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (nir_deref_mode_is_in_set(deref, modes)) {
      lower_explicit_io_deref(&b, deref, addr_format);
      }
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
   case nir_intrinsic_load_deref_block_intel:
   case nir_intrinsic_store_deref_block_intel:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (nir_deref_mode_is_in_set(deref, modes)) {
      lower_explicit_io_access(&b, intrin, addr_format);
                        case nir_intrinsic_deref_buffer_array_length: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (nir_deref_mode_is_in_set(deref, modes)) {
      lower_explicit_io_array_length(&b, intrin, addr_format);
                        case nir_intrinsic_deref_mode_is: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (nir_deref_mode_is_in_set(deref, modes)) {
      lower_explicit_io_mode_check(&b, intrin, addr_format);
                        case nir_intrinsic_launch_mesh_workgroups_with_payload_deref: {
      if (modes & nir_var_mem_task_payload) {
      /* Get address and size of the payload variable. */
   nir_deref_instr *deref = nir_src_as_deref(intrin->src[1]);
                        /* Replace the current instruction with the explicit intrinsic. */
   nir_def *dispatch_3d = intrin->src[0].ssa;
   b.cursor = nir_instr_remove(instr);
                                 default:
         }
               default:
      /* Nothing to do */
                     if (progress) {
         } else {
                     }
      /** Lower explicitly laid out I/O access to byte offset/address intrinsics
   *
   * This pass is intended to be used for any I/O which touches memory external
   * to the shader or which is directly visible to the client.  It requires that
   * all data types in the given modes have a explicit stride/offset decorations
   * to tell it exactly how to calculate the offset/address for the given load,
   * store, or atomic operation.  If the offset/stride information does not come
   * from the client explicitly (as with shared variables in GL or Vulkan),
   * nir_lower_vars_to_explicit_types() can be used to add them.
   *
   * Unlike nir_lower_io, this pass is fully capable of handling incomplete
   * pointer chains which may contain cast derefs.  It does so by walking the
   * deref chain backwards and simply replacing each deref, one at a time, with
   * the appropriate address calculation.  The pass takes a nir_address_format
   * parameter which describes how the offset or address is to be represented
   * during calculations.  By ensuring that the address is always in a
   * consistent format, pointers can safely be conjured from thin air by the
   * driver, stored to variables, passed through phis, etc.
   *
   * The one exception to the simple algorithm described above is for handling
   * row-major matrices in which case we may look down one additional level of
   * the deref chain.
   *
   * This pass is also capable of handling OpenCL generic pointers.  If the
   * address mode is global, it will lower any ambiguous (more than one mode)
   * access to global and pass through the deref_mode_is run-time checks as
   * addr_mode_is.  This assumes the driver has somehow mapped shared and
   * scratch memory to the global address space.  For other modes such as
   * 62bit_generic, there is an enum embedded in the address and we lower
   * ambiguous access to an if-ladder and deref_mode_is to a check against the
   * embedded enum.  If nir_lower_explicit_io is called on any shader that
   * contains generic pointers, it must either be used on all of the generic
   * modes or none.
   */
   bool
   nir_lower_explicit_io(nir_shader *shader, nir_variable_mode modes,
         {
               nir_foreach_function_impl(impl, shader) {
      if (impl && nir_lower_explicit_io_impl(impl, modes, addr_format))
                  }
      static bool
   nir_lower_vars_to_explicit_types_impl(nir_function_impl *impl,
               {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_deref_instr *deref = nir_instr_as_deref(instr);
                  unsigned size, alignment;
   const struct glsl_type *new_type =
         if (new_type != deref->type) {
      progress = true;
      }
   if (deref->deref_type == nir_deref_type_cast) {
      /* See also glsl_type::get_explicit_type_for_size_align() */
   unsigned new_stride = align(size, alignment);
   if (new_stride != deref->cast.ptr_stride) {
      deref->cast.ptr_stride = new_stride;
                        if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
                  } else {
                     }
      static bool
   lower_vars_to_explicit(nir_shader *shader,
               {
      bool progress = false;
   unsigned offset;
   switch (mode) {
   case nir_var_uniform:
      assert(shader->info.stage == MESA_SHADER_KERNEL);
   offset = 0;
      case nir_var_function_temp:
   case nir_var_shader_temp:
      offset = shader->scratch_size;
      case nir_var_mem_shared:
      offset = shader->info.shared_size;
      case nir_var_mem_task_payload:
      offset = shader->info.task_payload_size;
      case nir_var_mem_node_payload:
      assert(!shader->info.cs.node_payloads_size);
   offset = 0;
      case nir_var_mem_global:
      offset = shader->global_mem_size;
      case nir_var_mem_constant:
      offset = shader->constant_data_size;
      case nir_var_shader_call_data:
   case nir_var_ray_hit_attrib:
   case nir_var_mem_node_payload_in:
      offset = 0;
      default:
         }
   nir_foreach_variable_in_list(var, vars) {
      if (var->data.mode != mode)
            unsigned size, align;
   const struct glsl_type *explicit_type =
            if (explicit_type != var->type)
            UNUSED bool is_empty_struct =
                  assert(util_is_power_of_two_nonzero(align) || is_empty_struct ||
         var->data.driver_location = ALIGN_POT(offset, align);
   offset = var->data.driver_location + size;
               switch (mode) {
   case nir_var_uniform:
      assert(shader->info.stage == MESA_SHADER_KERNEL);
   shader->num_uniforms = offset;
      case nir_var_shader_temp:
   case nir_var_function_temp:
      shader->scratch_size = offset;
      case nir_var_mem_shared:
      shader->info.shared_size = offset;
      case nir_var_mem_task_payload:
      shader->info.task_payload_size = offset;
      case nir_var_mem_node_payload:
      shader->info.cs.node_payloads_size = offset;
      case nir_var_mem_global:
      shader->global_mem_size = offset;
      case nir_var_mem_constant:
      shader->constant_data_size = offset;
      case nir_var_shader_call_data:
   case nir_var_ray_hit_attrib:
   case nir_var_mem_node_payload_in:
         default:
                     }
      /* If nir_lower_vars_to_explicit_types is called on any shader that contains
   * generic pointers, it must either be used on all of the generic modes or
   * none.
   */
   bool
   nir_lower_vars_to_explicit_types(nir_shader *shader,
               {
      /* TODO: Situations which need to be handled to support more modes:
   * - row-major matrices
   * - compact shader inputs/outputs
   * - interface types
   */
   ASSERTED nir_variable_mode supported =
      nir_var_mem_shared | nir_var_mem_global | nir_var_mem_constant |
   nir_var_shader_temp | nir_var_function_temp | nir_var_uniform |
   nir_var_shader_call_data | nir_var_ray_hit_attrib |
   nir_var_mem_task_payload | nir_var_mem_node_payload |
                        if (modes & nir_var_uniform)
         if (modes & nir_var_mem_global)
            if (modes & nir_var_mem_shared) {
      assert(!shader->info.shared_memory_explicit_layout);
               if (modes & nir_var_shader_temp)
         if (modes & nir_var_mem_constant)
         if (modes & nir_var_shader_call_data)
         if (modes & nir_var_ray_hit_attrib)
         if (modes & nir_var_mem_task_payload)
         if (modes & nir_var_mem_node_payload)
         if (modes & nir_var_mem_node_payload_in)
            nir_foreach_function_impl(impl, shader) {
      if (modes & nir_var_function_temp)
                           }
      static void
   write_constant(void *dst, size_t dst_size,
         {
      if (c->is_null_constant) {
      memset(dst, 0, dst_size);
               if (glsl_type_is_vector_or_scalar(type)) {
      const unsigned num_components = glsl_get_vector_elements(type);
   const unsigned bit_size = glsl_get_bit_size(type);
   if (bit_size == 1) {
      /* Booleans are special-cased to be 32-bit
   *
   * TODO: Make the native bool bit_size an option.
   */
   assert(num_components * 4 <= dst_size);
   for (unsigned i = 0; i < num_components; i++) {
      int32_t b32 = -(int)c->values[i].b;
         } else {
      assert(bit_size >= 8 && bit_size % 8 == 0);
   const unsigned byte_size = bit_size / 8;
   assert(num_components * byte_size <= dst_size);
   for (unsigned i = 0; i < num_components; i++) {
      /* Annoyingly, thanks to packed structs, we can't make any
   * assumptions about the alignment of dst.  To avoid any strange
   * issues with unaligned writes, we always use memcpy.
   */
            } else if (glsl_type_is_array_or_matrix(type)) {
      const unsigned array_len = glsl_get_length(type);
   const unsigned stride = glsl_get_explicit_stride(type);
   assert(stride > 0);
   const struct glsl_type *elem_type = glsl_get_array_element(type);
   for (unsigned i = 0; i < array_len; i++) {
      unsigned elem_offset = i * stride;
   assert(elem_offset < dst_size);
   write_constant((char *)dst + elem_offset, dst_size - elem_offset,
         } else {
      assert(glsl_type_is_struct_or_ifc(type));
   const unsigned num_fields = glsl_get_length(type);
   for (unsigned i = 0; i < num_fields; i++) {
      const int field_offset = glsl_get_struct_field_offset(type, i);
   assert(field_offset >= 0 && field_offset < dst_size);
   const struct glsl_type *field_type = glsl_get_struct_field(type, i);
   write_constant((char *)dst + field_offset, dst_size - field_offset,
            }
      void
   nir_gather_explicit_io_initializers(nir_shader *shader,
               {
      /* It doesn't really make sense to gather initializers for more than one
   * mode at a time.  If this ever becomes well-defined, we can drop the
   * assert then.
   */
            nir_foreach_variable_with_modes(var, shader, mode) {
      assert(var->data.driver_location < dst_size);
   write_constant((char *)dst + var->data.driver_location,
               }
      /**
   * Return the offset source number for a load/store intrinsic or -1 if there's no offset.
   */
   int
   nir_get_io_offset_src_number(const nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_task_payload:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_kernel_input:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_2x32:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_fs_input_interp_deltas:
   case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
   case nir_intrinsic_task_payload_atomic:
   case nir_intrinsic_task_payload_atomic_swap:
   case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
         case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_primitive_output:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_task_payload:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_global_2x32:
   case nir_intrinsic_store_scratch:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
         case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_primitive_output:
         default:
            }
      /**
   * Return the offset source for a load/store intrinsic.
   */
   nir_src *
   nir_get_io_offset_src(nir_intrinsic_instr *instr)
   {
      const int idx = nir_get_io_offset_src_number(instr);
      }
      /**
   * Return the vertex index source number for a load/store per_vertex intrinsic or -1 if there's no offset.
   */
   int
   nir_get_io_arrayed_index_src_number(const nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_primitive_output:
         case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_primitive_output:
         default:
            }
      /**
   * Return the vertex index source for a load/store per_vertex intrinsic.
   */
   nir_src *
   nir_get_io_arrayed_index_src(nir_intrinsic_instr *instr)
   {
      const int idx = nir_get_io_arrayed_index_src_number(instr);
      }
      /**
   * Return the numeric constant that identify a NULL pointer for each address
   * format.
   */
   const nir_const_value *
   nir_address_format_null_value(nir_address_format addr_format)
   {
      const static nir_const_value null_values[][NIR_MAX_VEC_COMPONENTS] = {
      [nir_address_format_32bit_global] = { { 0 } },
   [nir_address_format_2x32bit_global] = { { 0 } },
   [nir_address_format_64bit_global] = { { 0 } },
   [nir_address_format_64bit_global_32bit_offset] = { { 0 } },
   [nir_address_format_64bit_bounded_global] = { { 0 } },
   [nir_address_format_32bit_index_offset] = { { .u32 = ~0 }, { .u32 = ~0 } },
   [nir_address_format_32bit_index_offset_pack64] = { { .u64 = ~0ull } },
   [nir_address_format_vec2_index_32bit_offset] = { { .u32 = ~0 }, { .u32 = ~0 }, { .u32 = ~0 } },
   [nir_address_format_32bit_offset] = { { .u32 = ~0 } },
   [nir_address_format_32bit_offset_as_64bit] = { { .u64 = ~0ull } },
   [nir_address_format_62bit_generic] = { { .u64 = 0 } },
               assert(addr_format < ARRAY_SIZE(null_values));
      }
      nir_def *
   nir_build_addr_ieq(nir_builder *b, nir_def *addr0, nir_def *addr1,
         {
      switch (addr_format) {
   case nir_address_format_32bit_global:
   case nir_address_format_2x32bit_global:
   case nir_address_format_64bit_global:
   case nir_address_format_64bit_bounded_global:
   case nir_address_format_32bit_index_offset:
   case nir_address_format_vec2_index_32bit_offset:
   case nir_address_format_32bit_offset:
   case nir_address_format_62bit_generic:
            case nir_address_format_64bit_global_32bit_offset:
      return nir_ball_iequal(b, nir_channels(b, addr0, 0xb),
         case nir_address_format_32bit_offset_as_64bit:
      assert(addr0->num_components == 1 && addr1->num_components == 1);
         case nir_address_format_32bit_index_offset_pack64:
      assert(addr0->num_components == 1 && addr1->num_components == 1);
         case nir_address_format_logical:
                     }
      nir_def *
   nir_build_addr_isub(nir_builder *b, nir_def *addr0, nir_def *addr1,
         {
      switch (addr_format) {
   case nir_address_format_32bit_global:
   case nir_address_format_64bit_global:
   case nir_address_format_32bit_offset:
   case nir_address_format_32bit_index_offset_pack64:
   case nir_address_format_62bit_generic:
      assert(addr0->num_components == 1);
   assert(addr1->num_components == 1);
         case nir_address_format_2x32bit_global:
      return nir_isub(b, addr_to_global(b, addr0, addr_format),
         case nir_address_format_32bit_offset_as_64bit:
      assert(addr0->num_components == 1);
   assert(addr1->num_components == 1);
         case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      return nir_isub(b, addr_to_global(b, addr0, addr_format),
         case nir_address_format_32bit_index_offset:
      assert(addr0->num_components == 2);
   assert(addr1->num_components == 2);
   /* Assume the same buffer index. */
         case nir_address_format_vec2_index_32bit_offset:
      assert(addr0->num_components == 3);
   assert(addr1->num_components == 3);
   /* Assume the same buffer index. */
         case nir_address_format_logical:
                     }
      static bool
   is_input(nir_intrinsic_instr *intrin)
   {
      return intrin->intrinsic == nir_intrinsic_load_input ||
         intrin->intrinsic == nir_intrinsic_load_input_vertex ||
   intrin->intrinsic == nir_intrinsic_load_per_vertex_input ||
      }
      static bool
   is_output(nir_intrinsic_instr *intrin)
   {
      return intrin->intrinsic == nir_intrinsic_load_output ||
         intrin->intrinsic == nir_intrinsic_load_per_vertex_output ||
   intrin->intrinsic == nir_intrinsic_load_per_primitive_output ||
   intrin->intrinsic == nir_intrinsic_store_output ||
      }
      static bool
   is_dual_slot(nir_intrinsic_instr *intrin)
   {
      if (intrin->intrinsic == nir_intrinsic_store_output ||
      intrin->intrinsic == nir_intrinsic_store_per_vertex_output ||
   intrin->intrinsic == nir_intrinsic_store_per_primitive_output) {
   return nir_src_bit_size(intrin->src[0]) == 64 &&
               return intrin->def.bit_size == 64 &&
      }
      /**
   * This pass adds constant offsets to instr->const_index[0] for input/output
   * intrinsics, and resets the offset source to 0.  Non-constant offsets remain
   * unchanged - since we don't know what part of a compound variable is
   * accessed, we allocate storage for the entire thing. For drivers that use
   * nir_lower_io_to_temporaries() before nir_lower_io(), this guarantees that
   * the offset source will be 0, so that they don't have to add it in manually.
   */
      static bool
   add_const_offset_to_base_block(nir_block *block, nir_builder *b,
         {
      bool progress = false;
   nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                     if (((modes & nir_var_shader_in) && is_input(intrin)) ||
                     /* NV_mesh_shader: ignore MS primitive indices. */
   if (b->shader->info.stage == MESA_SHADER_MESH &&
      sem.location == VARYING_SLOT_PRIMITIVE_INDICES &&
   !(b->shader->info.per_primitive_outputs &
                        /* TODO: Better handling of per-view variables here */
   if (nir_src_is_const(*offset) &&
                              sem.location += off;
   /* non-indirect indexing should reduce num_slots */
                  b->cursor = nir_before_instr(&intrin->instr);
   nir_src_rewrite(offset, nir_imm_int(b, 0));
                        }
      bool
   nir_io_add_const_offset_to_base(nir_shader *nir, nir_variable_mode modes)
   {
               nir_foreach_function_impl(impl, nir) {
      bool impl_progress = false;
   nir_builder b = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
         }
   progress |= impl_progress;
   if (impl_progress)
         else
                  }
      bool
   nir_lower_color_inputs(nir_shader *nir)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
                     nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              if (intrin->intrinsic != nir_intrinsic_load_input &&
                           if (sem.location != VARYING_SLOT_COL0 &&
                  /* Default to FLAT (for load_input) */
   enum glsl_interp_mode interp = INTERP_MODE_FLAT;
                  if (intrin->intrinsic == nir_intrinsic_load_interpolated_input) {
                     centroid =
         sample =
                                                   if (sem.location == VARYING_SLOT_COL0) {
      load = nir_load_color0(&b);
   nir->info.fs.color0_interp = interp;
   nir->info.fs.color0_sample = sample;
      } else {
      assert(sem.location == VARYING_SLOT_COL1);
   load = nir_load_color1(&b);
   nir->info.fs.color1_interp = interp;
   nir->info.fs.color1_sample = sample;
               if (intrin->num_components != 4) {
      unsigned start = nir_intrinsic_component(intrin);
   unsigned count = intrin->num_components;
               nir_def_rewrite_uses(&intrin->def, load);
   nir_instr_remove(instr);
                  if (progress) {
      nir_metadata_preserve(impl, nir_metadata_dominance |
      } else {
         }
      }
      bool
   nir_io_add_intrinsic_xfb_info(nir_shader *nir)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
            for (unsigned i = 0; i < NIR_MAX_XFB_BUFFERS; i++)
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                             /* No indirect indexing allowed. The index is implied to be 0. */
                  /* Calling this pass for the second time shouldn't do anything. */
   if (nir_intrinsic_io_xfb(intr).out[0].num_components ||
      nir_intrinsic_io_xfb(intr).out[1].num_components ||
   nir_intrinsic_io_xfb2(intr).out[0].num_components ||
                                             for (unsigned i = 0; i < nir->xfb_info->output_count; i++) {
      nir_xfb_output_info *out = &nir->xfb_info->outputs[i];
                     /*fprintf(stdout, "output%u: buffer=%u, offset=%u, location=%u, "
               i, out->buffer,
   out->offset,
                                                xfb[start / 2].out[start % 2].num_components = count;
   xfb[start / 2].out[start % 2].buffer = out->buffer;
   /* out->offset is relative to the first stored xfb component */
                                          nir_intrinsic_set_io_xfb(intr, xfb[0]);
                  nir_metadata_preserve(impl, nir_metadata_all);
      }
      static int
   type_size_vec4(const struct glsl_type *type, bool bindless)
   {
         }
      /**
   * This runs all compiler passes needed to lower IO, lower indirect IO access,
   * set transform feedback info in IO intrinsics, and clean up the IR.
   *
   * \param renumber_vs_inputs
   *    Set to true if holes between VS inputs should be removed, which is safe
   *    to do in any shader linker that can handle that. Set to false if you want
   *    to keep holes between VS inputs, which is recommended to do in gallium
   *    drivers so as not to break the mapping of vertex elements to VS inputs
   *    expected by gallium frontends.
   */
   void
   nir_lower_io_passes(nir_shader *nir, bool renumber_vs_inputs)
   {
      if (!nir->options->lower_io_variables ||
      nir->info.stage == MESA_SHADER_COMPUTE)
         bool has_indirect_inputs =
            /* Transform feedback requires that indirect outputs are lowered. */
   bool has_indirect_outputs =
      (nir->options->support_indirect_outputs >> nir->info.stage) & 0x1 &&
         /* TODO: Sorting variables by location is required due to some bug
   * in nir_lower_io_to_temporaries. If variables are not sorted,
   * dEQP-GLES31.functional.separate_shader.random.0 fails.
   *
   * This isn't needed if nir_assign_io_var_locations is called because it
   * also sorts variables. However, if IO is lowered sooner than that, we
   * must sort explicitly here to get what nir_assign_io_var_locations does.
   */
   unsigned varying_var_mask =
      (nir->info.stage != MESA_SHADER_VERTEX ? nir_var_shader_in : 0) |
               if (!has_indirect_inputs || !has_indirect_outputs) {
      NIR_PASS_V(nir, nir_lower_io_to_temporaries,
                  /* We need to lower all the copy_deref's introduced by lower_io_to-
   * _temporaries before calling nir_lower_io.
   */
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
               NIR_PASS_V(nir, nir_lower_io, nir_var_shader_out | nir_var_shader_in,
            /* nir_io_add_const_offset_to_base needs actual constants. */
   NIR_PASS_V(nir, nir_opt_constant_folding);
            /* Lower and remove dead derefs and variables to clean up the IR. */
   NIR_PASS_V(nir, nir_lower_vars_to_ssa);
   NIR_PASS_V(nir, nir_opt_dce);
            /* If IO is lowered before var->data.driver_location is assigned, driver
   * locations are all 0, which means IO bases are all 0. It's not necessary
   * to set driver_location before lowering IO because the only thing that
   * identifies outputs is their semantic, and IO bases can always be
   * computed from the semantics.
   *
   * This assigns IO bases from scratch, using IO semantics to tell which
   * intrinsics refer to the same IO. If the bases already exist, they
   * will be reassigned, sorted by the semantic, and all holes removed.
   * This kind of canonicalizes all bases.
   *
   * This must be done after DCE to remove dead load_input intrinsics.
   */
   NIR_PASS_V(nir, nir_recompute_io_bases,
                  if (nir->xfb_info)
               }
