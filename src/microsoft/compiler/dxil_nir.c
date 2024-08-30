   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dxil_nir.h"
   #include "dxil_module.h"
      #include "nir_builder.h"
   #include "nir_deref.h"
   #include "nir_worklist.h"
   #include "nir_to_dxil.h"
   #include "util/u_math.h"
   #include "vulkan/vulkan_core.h"
      static void
   cl_type_size_align(const struct glsl_type *type, unsigned *size,
         {
      *size = glsl_get_cl_size(type);
      }
      static nir_def *
   load_comps_to_vec(nir_builder *b, unsigned src_bit_size,
               {
      if (src_bit_size == dst_bit_size)
         else if (src_bit_size > dst_bit_size)
            unsigned num_dst_comps = DIV_ROUND_UP(num_src_comps * src_bit_size, dst_bit_size);
   unsigned comps_per_dst = dst_bit_size / src_bit_size;
            for (unsigned i = 0; i < num_dst_comps; i++) {
               dst_comps[i] = nir_u2uN(b, src_comps[src_offs], dst_bit_size);
   for (unsigned j = 1; j < comps_per_dst && src_offs + j < num_src_comps; j++) {
      nir_def *tmp = nir_ishl_imm(b, nir_u2uN(b, src_comps[src_offs + j], dst_bit_size),
                           }
      static bool
   lower_32b_offset_load(nir_builder *b, nir_intrinsic_instr *intr, nir_variable *var)
   {
      unsigned bit_size = intr->def.bit_size;
   unsigned num_components = intr->def.num_components;
                     nir_def *offset = intr->src[0].ssa;
   if (intr->intrinsic == nir_intrinsic_load_shared)
         else
         nir_def *index = nir_ushr_imm(b, offset, 2);
   nir_def *comps[NIR_MAX_VEC_COMPONENTS];
            /* We need to split loads in 32-bit accesses because the buffer
   * is an i32 array and DXIL does not support type casts.
   */
   unsigned num_32bit_comps = DIV_ROUND_UP(num_bits, 32);
   for (unsigned i = 0; i < num_32bit_comps; i++)
                  for (unsigned i = 0; i < num_32bit_comps; i += num_comps_per_pass) {
      unsigned num_vec32_comps = MIN2(num_32bit_comps - i, 4);
   unsigned num_dest_comps = num_vec32_comps * 32 / bit_size;
            /* If we have 16 bits or less to load we need to adjust the u32 value so
   * we can always extract the LSB.
   */
   if (num_bits <= 16) {
      nir_def *shift =
                     /* And now comes the pack/unpack step to match the original type. */
   unsigned dest_index = i * 32 / bit_size;
   nir_def *temp_vec = nir_extract_bits(b, &vec32, 1, 0, num_dest_comps, bit_size);
   for (unsigned comp = 0; comp < num_dest_comps; ++comp, ++dest_index)
               nir_def *result = nir_vec(b, comps, num_components);
   nir_def_rewrite_uses(&intr->def, result);
               }
      static void
   lower_masked_store_vec32(nir_builder *b, nir_def *offset, nir_def *index,
         {
               /* If we have small alignments, we need to place them correctly in the u32 component. */
   if (alignment <= 2) {
      nir_def *shift =
            vec32 = nir_ishl(b, vec32, shift);
               if (var->data.mode == nir_var_mem_shared) {
      /* Use the dedicated masked intrinsic */
   nir_deref_instr *deref = nir_build_deref_array(b, nir_build_deref_var(b, var), index);
   nir_deref_atomic(b, 32, &deref->def, nir_inot(b, mask), .atomic_op = nir_atomic_op_iand);
      } else {
      /* For scratch, since we don't need atomics, just generate the read-modify-write in NIR */
            nir_def *new_val = nir_ior(b, vec32,
                              }
      static bool
   lower_32b_offset_store(nir_builder *b, nir_intrinsic_instr *intr, nir_variable *var)
   {
      unsigned num_components = nir_src_num_components(intr->src[0]);
   unsigned bit_size = nir_src_bit_size(intr->src[0]);
                     nir_def *offset = intr->src[1].ssa;
   if (intr->intrinsic == nir_intrinsic_store_shared)
         else
                  unsigned comp_idx = 0;
   for (unsigned i = 0; i < num_components; i++)
            unsigned step = MAX2(bit_size, 32);
   for (unsigned i = 0; i < num_bits; i += step) {
      /* For each 4byte chunk (or smaller) we generate a 32bit scalar store. */
   unsigned substore_num_bits = MIN2(num_bits - i, step);
   nir_def *local_offset = nir_iadd_imm(b, offset, i / 8);
   nir_def *vec32 = load_comps_to_vec(b, bit_size, &comps[comp_idx],
                  /* For anything less than 32bits we need to use the masked version of the
   * intrinsic to preserve data living in the same 32bit slot. */
   if (substore_num_bits < 32) {
         } else {
      for (unsigned i = 0; i < vec32->num_components; ++i)
                                       }
      #define CONSTANT_LOCATION_UNVISITED 0
   #define CONSTANT_LOCATION_VALID 1
   #define CONSTANT_LOCATION_INVALID 2
      bool
   dxil_nir_lower_constant_to_temp(nir_shader *nir)
   {
      bool progress = false;
   nir_foreach_variable_with_modes(var, nir, nir_var_mem_constant)
      var->data.location = var->constant_initializer ?
         /* First pass: collect all UBO accesses that could be turned into
   * shader temp accesses.
   */
   nir_foreach_function(func, nir) {
      if (!func->is_entrypoint)
                  nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (!nir_deref_mode_is(deref, nir_var_mem_constant) ||
                        deref->var->data.location = nir_deref_instr_has_complex_use(deref, 0) ?
                     nir_foreach_variable_with_modes(var, nir, nir_var_mem_constant) {
      if (var->data.location != CONSTANT_LOCATION_VALID)
            /* Change the variable mode. */
                        /* Second pass: patch all derefs that were accessing the converted UBOs
   * variables.
   */
   nir_foreach_function(func, nir) {
      if (!func->is_entrypoint)
                  nir_builder b = nir_builder_create(func->impl);
   nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (nir_deref_mode_is(deref, nir_var_mem_constant)) {
      nir_deref_instr *parent = deref;
   while (parent && parent->deref_type != nir_deref_type_var)
         if (parent && parent->var->data.mode != nir_var_mem_constant) {
      deref->modes = parent->var->data.mode;
   /* Also change "pointer" size to 32-bit since this is now a logical pointer */
   deref->def.bit_size = 32;
   if (deref->deref_type == nir_deref_type_array) {
      b.cursor = nir_before_instr(instr);
                                 }
      static bool
   flatten_var_arrays(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
         default:
                  nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   nir_variable *var = NULL;
   for (nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      if (d->deref_type == nir_deref_type_cast)
         if (d->deref_type == nir_deref_type_var) {
      var = d->var;
   if (d->type == var->type)
         }
   if (!var)
            nir_deref_path path;
            assert(path.path[0]->deref_type == nir_deref_type_var);
   b->cursor = nir_before_instr(&path.path[0]->instr);
   nir_deref_instr *new_var_deref = nir_build_deref_var(b, var);
   nir_def *index = NULL;
   for (unsigned level = 1; path.path[level]; ++level) {
      nir_deref_instr *arr_deref = path.path[level];
   assert(arr_deref->deref_type == nir_deref_type_array);
   b->cursor = nir_before_instr(&arr_deref->instr);
   nir_def *val = nir_imul_imm(b, arr_deref->arr.index.ssa,
         if (index) {
         } else {
                     unsigned vector_comps = intr->num_components;
   if (vector_comps > 1) {
      b->cursor = nir_before_instr(&intr->instr);
   if (intr->intrinsic == nir_intrinsic_load_deref) {
      nir_def *components[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < vector_comps; ++i) {
      nir_def *final_index = index ? nir_iadd_imm(b, index, i) : nir_imm_int(b, i);
   nir_deref_instr *comp_deref = nir_build_deref_array(b, new_var_deref, final_index);
      }
      } else if (intr->intrinsic == nir_intrinsic_store_deref) {
      for (unsigned i = 0; i < vector_comps; ++i) {
      if (((1 << i) & nir_intrinsic_write_mask(intr)) == 0)
         nir_def *final_index = index ? nir_iadd_imm(b, index, i) : nir_imm_int(b, i);
   nir_deref_instr *comp_deref = nir_build_deref_array(b, new_var_deref, final_index);
         }
      } else {
                  nir_deref_path_finish(&path);
      }
      static void
   flatten_constant_initializer(nir_variable *var, nir_constant *src, nir_constant ***dest, unsigned vector_elements)
   {
      if (src->num_elements == 0) {
      for (unsigned i = 0; i < vector_elements; ++i) {
      nir_constant *new_scalar = rzalloc(var, nir_constant);
                  nir_constant **array_entry = (*dest)++;
         } else {
      for (unsigned i = 0; i < src->num_elements; ++i)
         }
      static bool
   flatten_var_array_types(nir_variable *var)
   {
      assert(!glsl_type_is_struct(glsl_without_array(var->type)));
   const struct glsl_type *matrix_type = glsl_without_array(var->type);
   if (!glsl_type_is_array_of_arrays(var->type) && glsl_get_components(matrix_type) == 1)
            enum glsl_base_type base_type = glsl_get_base_type(matrix_type);
   const struct glsl_type *flattened_type = glsl_array_type(glsl_scalar_type(base_type),
         var->type = flattened_type;
   if (var->constant_initializer) {
      nir_constant **new_elements = ralloc_array(var, nir_constant *, glsl_get_length(flattened_type));
   nir_constant **temp = new_elements;
   flatten_constant_initializer(var, var->constant_initializer, &temp, glsl_get_vector_elements(matrix_type));
   var->constant_initializer->num_elements = glsl_get_length(flattened_type);
      }
      }
      bool
   dxil_nir_flatten_var_arrays(nir_shader *shader, nir_variable_mode modes)
   {
      bool progress = false;
   nir_foreach_variable_with_modes(var, shader, modes & ~nir_var_function_temp)
            if (modes & nir_var_function_temp) {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_function_temp_variable(var, impl)
                  if (!progress)
            nir_shader_intrinsics_pass(shader, flatten_var_arrays,
                           nir_remove_dead_derefs(shader);
      }
      static bool
   lower_deref_bit_size(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
         default:
      /* Atomics can't be smaller than 32-bit */
               nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   /* Only interested in full deref chains */
   if (!var)
            const struct glsl_type *var_scalar_type = glsl_without_array(var->type);
   if (deref->type == var_scalar_type || !glsl_type_is_scalar(var_scalar_type))
            assert(deref->deref_type == nir_deref_type_var || deref->deref_type == nir_deref_type_array);
   const struct glsl_type *old_glsl_type = deref->type;
   nir_alu_type old_type = nir_get_nir_type_for_glsl_type(old_glsl_type);
   nir_alu_type new_type = nir_get_nir_type_for_glsl_type(var_scalar_type);
   if (glsl_get_bit_size(old_glsl_type) < glsl_get_bit_size(var_scalar_type)) {
      deref->type = var_scalar_type;
   if (intr->intrinsic == nir_intrinsic_load_deref) {
      intr->def.bit_size = glsl_get_bit_size(var_scalar_type);
   b->cursor = nir_after_instr(&intr->instr);
   nir_def *downcast = nir_type_convert(b, &intr->def, new_type, old_type, nir_rounding_mode_undef);
      }
   else {
      b->cursor = nir_before_instr(&intr->instr);
   nir_def *upcast = nir_type_convert(b, intr->src[1].ssa, old_type, new_type, nir_rounding_mode_undef);
               while (deref->deref_type == nir_deref_type_array) {
      nir_deref_instr *parent = nir_deref_instr_parent(deref);
   parent->type = glsl_type_wrap_in_arrays(deref->type, parent->type);
         } else {
      /* Assumed arrays are already flattened */
   b->cursor = nir_before_instr(&deref->instr);
   nir_deref_instr *parent = nir_build_deref_var(b, var);
   if (deref->deref_type == nir_deref_type_array)
         else
         nir_deref_instr *deref2 = nir_build_deref_array(b, parent,
         b->cursor = nir_before_instr(&intr->instr);
   if (intr->intrinsic == nir_intrinsic_load_deref) {
      nir_def *src1 = nir_load_deref(b, deref);
   nir_def *src2 = nir_load_deref(b, deref2);
      } else {
      nir_def *src1 = nir_unpack_64_2x32_split_x(b, intr->src[1].ssa);
   nir_def *src2 = nir_unpack_64_2x32_split_y(b, intr->src[1].ssa);
   nir_store_deref(b, deref, src1, 1);
      }
      }
      }
      static bool
   lower_var_bit_size_types(nir_variable *var, unsigned min_bit_size, unsigned max_bit_size)
   {
      assert(!glsl_type_is_array_of_arrays(var->type) && !glsl_type_is_struct(var->type));
   const struct glsl_type *type = glsl_without_array(var->type);
   assert(glsl_type_is_scalar(type));
   enum glsl_base_type base_type = glsl_get_base_type(type);
   if (glsl_base_type_get_bit_size(base_type) < min_bit_size) {
      switch (min_bit_size) {
   case 16:
      switch (base_type) {
   case GLSL_TYPE_BOOL:
      base_type = GLSL_TYPE_UINT16;
   for (unsigned i = 0; i < (var->constant_initializer ? var->constant_initializer->num_elements : 0); ++i)
            case GLSL_TYPE_INT8:
      base_type = GLSL_TYPE_INT16;
   for (unsigned i = 0; i < (var->constant_initializer ? var->constant_initializer->num_elements : 0); ++i)
            case GLSL_TYPE_UINT8: base_type = GLSL_TYPE_UINT16; break;
   default: unreachable("Unexpected base type");
   }
      case 32:
      switch (base_type) {
   case GLSL_TYPE_BOOL:
      base_type = GLSL_TYPE_UINT;
   for (unsigned i = 0; i < (var->constant_initializer ? var->constant_initializer->num_elements : 0); ++i)
            case GLSL_TYPE_INT8:
      base_type = GLSL_TYPE_INT;
   for (unsigned i = 0; i < (var->constant_initializer ? var->constant_initializer->num_elements : 0); ++i)
            case GLSL_TYPE_INT16:
      base_type = GLSL_TYPE_INT;
   for (unsigned i = 0; i < (var->constant_initializer ? var->constant_initializer->num_elements : 0); ++i)
            case GLSL_TYPE_FLOAT16:
      base_type = GLSL_TYPE_FLOAT;
   for (unsigned i = 0; i < (var->constant_initializer ? var->constant_initializer->num_elements : 0); ++i)
            case GLSL_TYPE_UINT8: base_type = GLSL_TYPE_UINT; break;
   case GLSL_TYPE_UINT16: base_type = GLSL_TYPE_UINT; break;
   default: unreachable("Unexpected base type");
   }
      default: unreachable("Unexpected min bit size");
   }
   var->type = glsl_type_wrap_in_arrays(glsl_scalar_type(base_type), var->type);
      }
   if (glsl_base_type_bit_size(base_type) > max_bit_size) {
      assert(!glsl_type_is_array_of_arrays(var->type));
   var->type = glsl_array_type(glsl_scalar_type(GLSL_TYPE_UINT),
               if (var->constant_initializer) {
      unsigned num_elements = var->constant_initializer->num_elements ?
         nir_constant **element_arr = ralloc_array(var, nir_constant *, num_elements);
   nir_constant *elements = rzalloc_array(var, nir_constant, num_elements);
   for (unsigned i = 0; i < var->constant_initializer->num_elements; ++i) {
      element_arr[i*2] = &elements[i*2];
   element_arr[i*2+1] = &elements[i*2+1];
   const nir_const_value *src = var->constant_initializer->num_elements ?
         elements[i*2].values[0].u32 = (uint32_t)src->u64;
   elements[i*2].is_null_constant = (uint32_t)src->u64 == 0;
   elements[i*2+1].values[0].u32 = (uint32_t)(src->u64 >> 32);
      }
   var->constant_initializer->num_elements = num_elements;
      }
      }
      }
      bool
   dxil_nir_lower_var_bit_size(nir_shader *shader, nir_variable_mode modes,
         {
      bool progress = false;
   nir_foreach_variable_with_modes(var, shader, modes & ~nir_var_function_temp)
            if (modes & nir_var_function_temp) {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_function_temp_variable(var, impl)
                  if (!progress)
            nir_shader_intrinsics_pass(shader, lower_deref_bit_size,
                           nir_remove_dead_derefs(shader);
      }
      static bool
   lower_shared_atomic(nir_builder *b, nir_intrinsic_instr *intr, nir_variable *var)
   {
               nir_def *offset =
                  nir_deref_instr *deref = nir_build_deref_array(b, nir_build_deref_var(b, var), index);
   nir_def *result;
   if (intr->intrinsic == nir_intrinsic_shared_atomic_swap)
      result = nir_deref_atomic_swap(b, 32, &deref->def, intr->src[1].ssa, intr->src[2].ssa,
      else
      result = nir_deref_atomic(b, 32, &deref->def, intr->src[1].ssa,
         nir_def_rewrite_uses(&intr->def, result);
   nir_instr_remove(&intr->instr);
      }
      bool
   dxil_nir_lower_loads_stores_to_dxil(nir_shader *nir,
         {
      bool progress = nir_remove_dead_variables(nir, nir_var_function_temp | nir_var_mem_shared, NULL);
   nir_variable *shared_var = NULL;
   if (nir->info.shared_size) {
      shared_var = nir_variable_create(nir, nir_var_mem_shared,
                     unsigned ptr_size = nir->info.cs.ptr_size;
   if (nir->info.stage == MESA_SHADER_KERNEL) {
      /* All the derefs created here will be used as GEP indices so force 32-bit */
      }
   nir_foreach_function_impl(impl, nir) {
               nir_variable *scratch_var = NULL;
   if (nir->scratch_size) {
      const struct glsl_type *scratch_type = glsl_array_type(glsl_uint_type(), DIV_ROUND_UP(nir->scratch_size, 4), 4);
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                  switch (intr->intrinsic) {
   case nir_intrinsic_load_shared:
      progress |= lower_32b_offset_load(&b, intr, shared_var);
      case nir_intrinsic_load_scratch:
      progress |= lower_32b_offset_load(&b, intr, scratch_var);
      case nir_intrinsic_store_shared:
      progress |= lower_32b_offset_store(&b, intr, shared_var);
      case nir_intrinsic_store_scratch:
      progress |= lower_32b_offset_store(&b, intr, scratch_var);
      case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
      progress |= lower_shared_atomic(&b, intr, shared_var);
      default:
                  }
   if (nir->info.stage == MESA_SHADER_KERNEL) {
                     }
      static bool
   lower_deref_ssbo(nir_builder *b, nir_deref_instr *deref)
   {
      assert(nir_deref_mode_is(deref, nir_var_mem_ssbo));
   assert(deref->deref_type == nir_deref_type_var ||
                           if (deref->deref_type == nir_deref_type_var) {
      /* We turn all deref_var into deref_cast and build a pointer value based on
   * the var binding which encodes the UAV id.
   */
   nir_def *ptr = nir_imm_int64(b, (uint64_t)var->data.binding << 32);
   nir_deref_instr *deref_cast =
      nir_build_deref_cast(b, ptr, nir_var_mem_ssbo, deref->type,
      nir_def_rewrite_uses(&deref->def,
                  deref = deref_cast;
      }
      }
      bool
   dxil_nir_lower_deref_ssbo(nir_shader *nir)
   {
               foreach_list_typed(nir_function, func, node, &nir->functions) {
      if (!func->is_entrypoint)
                           nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
                              if (!nir_deref_mode_is(deref, nir_var_mem_ssbo) ||
                                             }
      static bool
   lower_alu_deref_srcs(nir_builder *b, nir_alu_instr *alu)
   {
      const nir_op_info *info = &nir_op_infos[alu->op];
                     for (unsigned i = 0; i < info->num_inputs; i++) {
               if (!deref)
            nir_deref_path path;
   nir_deref_path_init(&path, deref, NULL);
   nir_deref_instr *root_deref = path.path[0];
            if (root_deref->deref_type != nir_deref_type_cast)
            nir_def *ptr =
      nir_iadd(b, root_deref->parent.ssa,
      nir_src_rewrite(&alu->src[i].src, ptr);
                  }
      bool
   dxil_nir_opt_alu_deref_srcs(nir_shader *nir)
   {
               foreach_list_typed(nir_function, func, node, &nir->functions) {
      if (!func->is_entrypoint)
                           nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_alu_instr *alu = nir_instr_as_alu(instr);
                        }
      static void
   cast_phi(nir_builder *b, nir_phi_instr *phi, unsigned new_bit_size)
   {
      nir_phi_instr *lowered = nir_phi_instr_create(b->shader);
   int num_components = 0;
            nir_foreach_phi_src(src, phi) {
      assert(num_components == 0 || num_components == src->src.ssa->num_components);
                                          nir_def_init(&lowered->instr, &lowered->def, num_components,
            b->cursor = nir_before_instr(&phi->instr);
            b->cursor = nir_after_phis(nir_cursor_current_block(b->cursor));
            nir_def_rewrite_uses(&phi->def, result);
      }
      static bool
   upcast_phi_impl(nir_function_impl *impl, unsigned min_bit_size)
   {
      nir_builder b = nir_builder_create(impl);
            nir_foreach_block_reverse(block, impl) {
      nir_foreach_phi_safe(phi, block) {
      if (phi->def.bit_size == 1 ||
                  cast_phi(&b, phi, min_bit_size);
                  if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   dxil_nir_lower_upcast_phis(nir_shader *shader, unsigned min_bit_size)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
      struct dxil_nir_split_clip_cull_distance_params {
      nir_variable *new_var[2];
      };
      /* In GLSL and SPIR-V, clip and cull distance are arrays of floats (with a limit of 8).
   * In DXIL, clip and cull distances are up to 2 float4s combined.
   * Coming from GLSL, we can request this 2 float4 format, but coming from SPIR-V,
   * we can't, and have to accept a "compact" array of scalar floats.
   *
   * To help emitting a valid input signature for this case, split the variables so that they
   * match what we need to put in the signature (e.g. { float clip[4]; float clip1; float cull[3]; })
   */
   static bool
   dxil_nir_split_clip_cull_distance_instr(nir_builder *b,
               {
               if (instr->type != nir_instr_type_deref)
            nir_deref_instr *deref = nir_instr_as_deref(instr);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   if (!var ||
      var->data.location < VARYING_SLOT_CLIP_DIST0 ||
   var->data.location > VARYING_SLOT_CULL_DIST1 ||
   !var->data.compact)
         unsigned new_var_idx = var->data.mode == nir_var_shader_in ? 0 : 1;
            /* The location should only be inside clip distance, because clip
   * and cull should've been merged by nir_lower_clip_cull_distance_arrays()
   */
   assert(var->data.location == VARYING_SLOT_CLIP_DIST0 ||
            /* The deref chain to the clip/cull variables should be simple, just the
   * var and an array with a constant index, otherwise more lowering/optimization
   * might be needed before this pass, e.g. copy prop, lower_io_to_temporaries,
   * split_var_copies, and/or lower_var_copies. In the case of arrayed I/O like
   * inputs to the tessellation or geometry stages, there might be a second level
   * of array index.
   */
   assert(deref->deref_type == nir_deref_type_var ||
            b->cursor = nir_before_instr(instr);
   unsigned arrayed_io_length = 0;
   const struct glsl_type *old_type = var->type;
   if (nir_is_arrayed_io(var, b->shader->info.stage)) {
      arrayed_io_length = glsl_array_size(old_type);
      }
   if (!new_var) {
      /* Update lengths for new and old vars */
   int old_length = glsl_array_size(old_type);
   int new_length = (old_length + var->data.location_frac) - 4;
            /* The existing variable fits in the float4 */
   if (new_length <= 0)
            new_var = nir_variable_clone(var, params->shader);
   nir_shader_add_variable(params->shader, new_var);
   assert(glsl_get_base_type(glsl_get_array_element(old_type)) == GLSL_TYPE_FLOAT);
   var->type = glsl_array_type(glsl_float_type(), old_length, 0);
   new_var->type = glsl_array_type(glsl_float_type(), new_length, 0);
   if (arrayed_io_length) {
      var->type = glsl_array_type(var->type, arrayed_io_length, 0);
      }
   new_var->data.location++;
   new_var->data.location_frac = 0;
               /* Update the type for derefs of the old var */
   if (deref->deref_type == nir_deref_type_var) {
      deref->type = var->type;
               if (glsl_type_is_array(deref->type)) {
      assert(arrayed_io_length > 0);
   deref->type = glsl_get_array_element(var->type);
                        nir_const_value *index = nir_src_as_const_value(deref->arr.index);
            /* Treat this array as a vector starting at the component index in location_frac,
   * so if location_frac is 1 and index is 0, then it's accessing the 'y' component
   * of the vector. If index + location_frac is >= 4, there's no component there,
   * so we need to add a new variable and adjust the index.
   */
   unsigned total_index = index->u32 + var->data.location_frac;
   if (total_index < 4)
            nir_deref_instr *new_var_deref = nir_build_deref_var(b, new_var);
   nir_deref_instr *new_intermediate_deref = new_var_deref;
   if (arrayed_io_length) {
      nir_deref_instr *parent = nir_src_as_deref(deref->parent);
   assert(parent->deref_type == nir_deref_type_array);
      }
   nir_deref_instr *new_array_deref = nir_build_deref_array(b, new_intermediate_deref, nir_imm_int(b, total_index % 4));
   nir_def_rewrite_uses(&deref->def, &new_array_deref->def);
      }
      bool
   dxil_nir_split_clip_cull_distance(nir_shader *shader)
   {
      struct dxil_nir_split_clip_cull_distance_params params = {
      .new_var = { NULL, NULL },
      };
   nir_shader_instructions_pass(shader,
                                    }
      static bool
   dxil_nir_lower_double_math_instr(nir_builder *b,
               {
      if (instr->type != nir_instr_type_alu)
                     /* TODO: See if we can apply this explicitly to packs/unpacks that are then
   * used as a double. As-is, if we had an app explicitly do a 64bit integer op,
   * then try to bitcast to double (not expressible in HLSL, but it is in other
   * source languages), this would unpack the integer and repack as a double, when
   * we probably want to just send the bitcast through to the backend.
                     bool progress = false;
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; ++i) {
      if (nir_alu_type_get_base_type(nir_op_infos[alu->op].input_types[i]) == nir_type_float &&
      alu->src[i].src.ssa->bit_size == 64) {
   unsigned num_components = nir_op_infos[alu->op].input_sizes[i];
   if (!num_components)
         nir_def *components[NIR_MAX_VEC_COMPONENTS];
   for (unsigned c = 0; c < num_components; ++c) {
      nir_def *packed_double = nir_channel(b, alu->src[i].src.ssa, alu->src[i].swizzle[c]);
   nir_def *unpacked_double = nir_unpack_64_2x32(b, packed_double);
   components[c] = nir_pack_double_2x32_dxil(b, unpacked_double);
      }
   nir_src_rewrite(&alu->src[i].src,
                        if (nir_alu_type_get_base_type(nir_op_infos[alu->op].output_type) == nir_type_float &&
      alu->def.bit_size == 64) {
   b->cursor = nir_after_instr(&alu->instr);
   nir_def *components[NIR_MAX_VEC_COMPONENTS];
   for (unsigned c = 0; c < alu->def.num_components; ++c) {
      nir_def *packed_double = nir_channel(b, &alu->def, c);
   nir_def *unpacked_double = nir_unpack_double_2x32_dxil(b, packed_double);
      }
   nir_def *repacked_dvec = nir_vec(b, components, alu->def.num_components);
   nir_def_rewrite_uses_after(&alu->def, repacked_dvec, repacked_dvec->parent_instr);
                  }
      bool
   dxil_nir_lower_double_math(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader,
                              }
      typedef struct {
      gl_system_value *values;
      } zero_system_values_state;
      static bool
   lower_system_value_to_zero_filter(const nir_instr* instr, const void* cb_state)
   {
      if (instr->type != nir_instr_type_intrinsic) {
                           /* All the intrinsics we care about are loads */
   if (!nir_intrinsic_infos[intrin->intrinsic].has_dest)
            zero_system_values_state* state = (zero_system_values_state*)cb_state;
   for (uint32_t i = 0; i < state->count; ++i) {
      gl_system_value value = state->values[i];
            if (intrin->intrinsic == value_op) {
         } else if (intrin->intrinsic == nir_intrinsic_load_deref) {
      nir_deref_instr* deref = nir_src_as_deref(intrin->src[0]);
                  nir_variable* var = deref->var;
   if (var->data.location == value) {
                           }
      static nir_def*
   lower_system_value_to_zero_instr(nir_builder* b, nir_instr* instr, void* _state)
   {
         }
      bool
   dxil_nir_lower_system_values_to_zero(nir_shader* shader,
               {
      zero_system_values_state state = { system_values, count };
   return nir_shader_lower_instructions(shader,
      lower_system_value_to_zero_filter,
   lower_system_value_to_zero_instr,
   }
      static void
   lower_load_local_group_size(nir_builder *b, nir_intrinsic_instr *intr)
   {
               nir_const_value v[3] = {
      nir_const_value_for_int(b->shader->info.workgroup_size[0], 32),
   nir_const_value_for_int(b->shader->info.workgroup_size[1], 32),
      };
   nir_def *size = nir_build_imm(b, 3, 32, v);
   nir_def_rewrite_uses(&intr->def, size);
      }
      static bool
   lower_system_values_impl(nir_builder *b, nir_intrinsic_instr *intr,
         {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_workgroup_size:
      lower_load_local_group_size(b, intr);
      default:
            }
      bool
   dxil_nir_lower_system_values(nir_shader *shader)
   {
      return nir_shader_intrinsics_pass(shader, lower_system_values_impl,
            }
      static const struct glsl_type *
   get_bare_samplers_for_type(const struct glsl_type *type, bool is_shadow)
   {
      const struct glsl_type *base_sampler_type =
      is_shadow ?
         }
      static const struct glsl_type *
   get_textures_for_sampler_type(const struct glsl_type *type)
   {
      return glsl_type_wrap_in_arrays(
      glsl_sampler_type_to_texture(
   }
      static bool
   redirect_sampler_derefs(struct nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_tex)
                     int sampler_idx = nir_tex_instr_src_index(tex, nir_tex_src_sampler_deref);
   if (sampler_idx == -1) {
      /* No sampler deref - does this instruction even need a sampler? If not,
   * sampler_index doesn't necessarily point to a sampler, so early-out.
   */
   if (!nir_tex_instr_need_sampler(tex))
            /* No derefs but needs a sampler, must be using indices */
            /* Already have a bare sampler here */
   if (bare_sampler)
            nir_variable *old_sampler = NULL;
   nir_foreach_variable_with_modes(var, b->shader, nir_var_uniform) {
      if (var->data.binding <= tex->sampler_index &&
                     /* Already have a bare sampler for this binding and it is of the
   * correct type, add it to the table */
   if (glsl_type_is_bare_sampler(glsl_without_array(var->type)) &&
      glsl_sampler_type_is_shadow(glsl_without_array(var->type)) ==
                                                   /* Clone the original sampler to a bare sampler of the correct type */
   bare_sampler = nir_variable_clone(old_sampler, b->shader);
            bare_sampler->type =
         _mesa_hash_table_u64_insert(data, tex->sampler_index, bare_sampler);
               /* Using derefs, means we have to rewrite the deref chain in addition to cloning */
   nir_deref_instr *final_deref = nir_src_as_deref(tex->src[sampler_idx].src);
   nir_deref_path path;
            nir_deref_instr *old_tail = path.path[0];
   assert(old_tail->deref_type == nir_deref_type_var);
   nir_variable *old_var = old_tail->var;
   if (glsl_type_is_bare_sampler(glsl_without_array(old_var->type)) &&
      glsl_sampler_type_is_shadow(glsl_without_array(old_var->type)) ==
         nir_deref_path_finish(&path);
               uint64_t var_key = ((uint64_t)old_var->data.descriptor_set << 32) |
         nir_variable *new_var = _mesa_hash_table_u64_search(data, var_key);
   if (!new_var) {
      new_var = nir_variable_clone(old_var, b->shader);
   nir_shader_add_variable(b->shader, new_var);
   new_var->type = 
                     b->cursor = nir_after_instr(&old_tail->instr);
            for (unsigned i = 1; path.path[i]; ++i) {
      b->cursor = nir_after_instr(&path.path[i]->instr);
               nir_deref_path_finish(&path);
   nir_src_rewrite(&tex->src[sampler_idx].src, &new_tail->def);
      }
      static bool
   redirect_texture_derefs(struct nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_tex)
                     int texture_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
   if (texture_idx == -1) {
      /* No derefs, must be using indices */
            /* Already have a texture here */
   if (bare_sampler)
            nir_variable *typed_sampler = NULL;
   nir_foreach_variable_with_modes(var, b->shader, nir_var_uniform) {
      if (var->data.binding <= tex->texture_index &&
      var->data.binding + glsl_type_get_texture_count(var->type) > tex->texture_index) {
   /* Already have a texture for this binding, add it to the table */
   _mesa_hash_table_u64_insert(data, tex->texture_index, var);
               if (var->data.binding <= tex->texture_index &&
      var->data.binding + glsl_type_get_sampler_count(var->type) > tex->texture_index &&
   !glsl_type_is_bare_sampler(glsl_without_array(var->type))) {
                  /* Clone the typed sampler to a texture and we're done */
   assert(typed_sampler);
   bare_sampler = nir_variable_clone(typed_sampler, b->shader);
   bare_sampler->type = get_textures_for_sampler_type(typed_sampler->type);
   nir_shader_add_variable(b->shader, bare_sampler);
   _mesa_hash_table_u64_insert(data, tex->texture_index, bare_sampler);
               /* Using derefs, means we have to rewrite the deref chain in addition to cloning */
   nir_deref_instr *final_deref = nir_src_as_deref(tex->src[texture_idx].src);
   nir_deref_path path;
            nir_deref_instr *old_tail = path.path[0];
   assert(old_tail->deref_type == nir_deref_type_var);
   nir_variable *old_var = old_tail->var;
   if (glsl_type_is_texture(glsl_without_array(old_var->type)) ||
      glsl_type_is_image(glsl_without_array(old_var->type))) {
   nir_deref_path_finish(&path);
               uint64_t var_key = ((uint64_t)old_var->data.descriptor_set << 32) |
         nir_variable *new_var = _mesa_hash_table_u64_search(data, var_key);
   if (!new_var) {
      new_var = nir_variable_clone(old_var, b->shader);
   new_var->type = get_textures_for_sampler_type(old_var->type);
   nir_shader_add_variable(b->shader, new_var);
               b->cursor = nir_after_instr(&old_tail->instr);
            for (unsigned i = 1; path.path[i]; ++i) {
      b->cursor = nir_after_instr(&path.path[i]->instr);
               nir_deref_path_finish(&path);
               }
      bool
   dxil_nir_split_typed_samplers(nir_shader *nir)
   {
               bool progress = nir_shader_instructions_pass(nir, redirect_sampler_derefs,
                     progress |= nir_shader_instructions_pass(nir, redirect_texture_derefs,
            _mesa_hash_table_u64_destroy(hash_table);
      }
         static bool
   lower_sysval_to_load_input_impl(nir_builder *b, nir_intrinsic_instr *intr,
         {
      gl_system_value sysval = SYSTEM_VALUE_MAX;
   switch (intr->intrinsic) {
   case nir_intrinsic_load_front_face:
      sysval = SYSTEM_VALUE_FRONT_FACE;
      case nir_intrinsic_load_instance_id:
      sysval = SYSTEM_VALUE_INSTANCE_ID;
      case nir_intrinsic_load_vertex_id_zero_base:
      sysval = SYSTEM_VALUE_VERTEX_ID_ZERO_BASE;
      default:
                  nir_variable **sysval_vars = (nir_variable **)data;
   nir_variable *var = sysval_vars[sysval];
            const nir_alu_type dest_type = (sysval == SYSTEM_VALUE_FRONT_FACE)
         const unsigned bit_size = (sysval == SYSTEM_VALUE_FRONT_FACE)
            b->cursor = nir_before_instr(&intr->instr);
   nir_def *result = nir_load_input(b, intr->def.num_components, bit_size, nir_imm_int(b, 0),
            /* The nir_type_uint32 is really a nir_type_bool32, but that type is very
   * inconvenient at this point during compilation.  Convert to
   * nir_type_bool1 by comparing with zero.
   */
   if (sysval == SYSTEM_VALUE_FRONT_FACE)
            nir_def_rewrite_uses(&intr->def, result);
      }
      bool
   dxil_nir_lower_sysval_to_load_input(nir_shader *s, nir_variable **sysval_vars)
   {
      return nir_shader_intrinsics_pass(s, lower_sysval_to_load_input_impl,
            }
      /* Comparison function to sort io values so that first come normal varyings,
   * then system values, and then system generated values.
   */
   static int
   variable_location_cmp(const nir_variable* a, const nir_variable* b)
   {
      // Sort by stream, driver_location, location, location_frac, then index
   unsigned a_location = a->data.location;
   if (a_location >= VARYING_SLOT_PATCH0)
         unsigned b_location = b->data.location;
   if (b_location >= VARYING_SLOT_PATCH0)
         unsigned a_stream = a->data.stream & ~NIR_STREAM_PACKED;
   unsigned b_stream = b->data.stream & ~NIR_STREAM_PACKED;
   return a_stream != b_stream ?
            a_stream - b_stream :
   a->data.driver_location != b->data.driver_location ?
      a->data.driver_location - b->data.driver_location :
   a_location !=  b_location ?
      a_location - b_location :
   }
      /* Order varyings according to driver location */
   uint64_t
   dxil_sort_by_driver_location(nir_shader* s, nir_variable_mode modes)
   {
               uint64_t result = 0;
   nir_foreach_variable_with_modes(var, s, modes) {
         }
      }
      /* Sort PS outputs so that color outputs come first */
   void
   dxil_sort_ps_outputs(nir_shader* s)
   {
      nir_foreach_variable_with_modes_safe(var, s, nir_var_shader_out) {
      /* We use the driver_location here to avoid introducing a new
   * struct or member variable here. The true, updated driver location
   * will be written below, after sorting */
   switch (var->data.location) {
   case FRAG_RESULT_DEPTH:
      var->data.driver_location = 1;
      case FRAG_RESULT_STENCIL:
      var->data.driver_location = 2;
      case FRAG_RESULT_SAMPLE_MASK:
      var->data.driver_location = 3;
      default:
                     nir_sort_variables_with_modes(s, variable_location_cmp,
            unsigned driver_loc = 0;
   nir_foreach_variable_with_modes(var, s, nir_var_shader_out) {
            }
      enum dxil_sysvalue_type {
      DXIL_NO_SYSVALUE = 0,
   DXIL_USED_SYSVALUE,
   DXIL_SYSVALUE,
      };
      static enum dxil_sysvalue_type
   nir_var_to_dxil_sysvalue_type(nir_variable *var, uint64_t other_stage_mask)
   {
      switch (var->data.location) {
   case VARYING_SLOT_FACE:
         case VARYING_SLOT_POS:
   case VARYING_SLOT_PRIMITIVE_ID:
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
   case VARYING_SLOT_PSIZ:
   case VARYING_SLOT_TESS_LEVEL_INNER:
   case VARYING_SLOT_TESS_LEVEL_OUTER:
   case VARYING_SLOT_VIEWPORT:
   case VARYING_SLOT_LAYER:
   case VARYING_SLOT_VIEW_INDEX:
      if (!((1ull << var->data.location) & other_stage_mask))
            default:
            }
      /* Order between stage values so that normal varyings come first,
   * then sysvalues and then system generated values.
   */
   uint64_t
   dxil_reassign_driver_locations(nir_shader* s, nir_variable_mode modes,
         {
      nir_foreach_variable_with_modes_safe(var, s, modes) {
      /* We use the driver_location here to avoid introducing a new
   * struct or member variable here. The true, updated driver location
   * will be written below, after sorting */
                        uint64_t result = 0;
   unsigned driver_loc = 0, driver_patch_loc = 0;
   nir_foreach_variable_with_modes(var, s, modes) {
      if (var->data.location < 64)
         /* Overlap patches with non-patch */
   var->data.driver_location = var->data.patch ?
      }
      }
      static bool
   lower_ubo_array_one_to_static(struct nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_load_vulkan_descriptor)
            nir_variable *var =
            if (!var)
            if (!glsl_type_is_array(var->type) || glsl_array_size(var->type) != 1)
            nir_intrinsic_instr *index = nir_src_as_intrinsic(intrin->src[0]);
   /* We currently do not support reindex */
            if (nir_src_is_const(index->src[0]) && nir_src_as_uint(index->src[0]) == 0)
            if (nir_intrinsic_desc_type(index) != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                     // Indexing out of bounds on array of UBOs is considered undefined
   // behavior. Therefore, we just hardcode all the index to 0.
   uint8_t bit_size = index->def.bit_size;
   nir_def *zero = nir_imm_intN_t(b, 0, bit_size);
   nir_def *dest =
      nir_vulkan_resource_index(b, index->num_components, bit_size, zero,
                                 }
      bool
   dxil_nir_lower_ubo_array_one_to_static(nir_shader *s)
   {
      bool progress = nir_shader_intrinsics_pass(s,
                     }
      static bool
   is_fquantize2f16(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
      }
      static nir_def *
   lower_fquantize2f16(struct nir_builder *b, nir_instr *instr, void *data)
   {
      /*
   * SpvOpQuantizeToF16 documentation says:
   *
   * "
   * If Value is an infinity, the result is the same infinity.
   * If Value is a NaN, the result is a NaN, but not necessarily the same NaN.
   * If Value is positive with a magnitude too large to represent as a 16-bit
   * floating-point value, the result is positive infinity. If Value is negative
   * with a magnitude too large to represent as a 16-bit floating-point value,
   * the result is negative infinity. If the magnitude of Value is too small to
   * represent as a normalized 16-bit floating-point value, the result may be
   * either +0 or -0.
   * "
   *
   * which we turn into:
   *
   *   if (val < MIN_FLOAT16)
   *      return -INFINITY;
   *   else if (val > MAX_FLOAT16)
   *      return -INFINITY;
   *   else if (fabs(val) < SMALLEST_NORMALIZED_FLOAT16 && sign(val) != 0)
   *      return -0.0f;
   *   else if (fabs(val) < SMALLEST_NORMALIZED_FLOAT16 && sign(val) == 0)
   *      return +0.0f;
   *   else
   *      return round(val);
   */
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   nir_def *src =
            nir_def *neg_inf_cond =
         nir_def *pos_inf_cond =
         nir_def *zero_cond =
         nir_def *zero = nir_iand_imm(b, src, 1 << 31);
            nir_def *res =
         res = nir_bcsel(b, pos_inf_cond, nir_imm_float(b, INFINITY), res);
   res = nir_bcsel(b, zero_cond, zero, res);
      }
      bool
   dxil_nir_lower_fquantize2f16(nir_shader *s)
   {
         }
      static bool
   fix_io_uint_deref_types(struct nir_builder *builder, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_deref)
            nir_deref_instr *deref = nir_instr_as_deref(instr);
            if (var == data) {
      deref->type = glsl_type_wrap_in_arrays(glsl_uint_type(), deref->type);
                  }
      static bool
   fix_io_uint_type(nir_shader *s, nir_variable_mode modes, int slot)
   {
      nir_variable *fixed_var = NULL;
   nir_foreach_variable_with_modes(var, s, modes) {
      if (var->data.location == slot) {
      const struct glsl_type *plain_type = glsl_without_array(var->type);
                  assert(plain_type == glsl_int_type());
   var->type = glsl_type_wrap_in_arrays(glsl_uint_type(), var->type);
   fixed_var = var;
                           return nir_shader_instructions_pass(s, fix_io_uint_deref_types,
      }
      bool
   dxil_nir_fix_io_uint_type(nir_shader *s, uint64_t in_mask, uint64_t out_mask)
   {
      if (!(s->info.outputs_written & out_mask) &&
      !(s->info.inputs_read & in_mask))
                  while (in_mask) {
      int slot = u_bit_scan64(&in_mask);
   progress |= (s->info.inputs_read & (1ull << slot)) &&
               while (out_mask) {
      int slot = u_bit_scan64(&out_mask);
   progress |= (s->info.outputs_written & (1ull << slot)) &&
                  }
      struct remove_after_discard_state {
         };
      static bool
   remove_after_discard(struct nir_builder *builder, nir_instr *instr,
         {
      struct remove_after_discard_state *state = cb_data;
   if (instr->block == state->active_block) {
      nir_instr_remove_v(instr);
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_discard &&
      intr->intrinsic != nir_intrinsic_terminate &&
   intr->intrinsic != nir_intrinsic_discard_if &&
   intr->intrinsic != nir_intrinsic_terminate_if)
                     }
      static bool
   lower_kill(struct nir_builder *builder, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_discard &&
      intr->intrinsic != nir_intrinsic_terminate &&
   intr->intrinsic != nir_intrinsic_discard_if &&
   intr->intrinsic != nir_intrinsic_terminate_if)
         builder->cursor = nir_instr_remove(&intr->instr);
   if (intr->intrinsic == nir_intrinsic_discard ||
      intr->intrinsic == nir_intrinsic_terminate) {
      } else {
                              }
      bool
   dxil_nir_lower_discard_and_terminate(nir_shader *s)
   {
      if (s->info.stage != MESA_SHADER_FRAGMENT)
            // This pass only works if all functions have been inlined
   assert(exec_list_length(&s->functions) == 1);
   struct remove_after_discard_state state;
   state.active_block = NULL;
   nir_shader_instructions_pass(s, remove_after_discard, nir_metadata_none,
         return nir_shader_intrinsics_pass(s, lower_kill, nir_metadata_none,
      }
      static bool
   update_writes(struct nir_builder *b, nir_intrinsic_instr *intr, void *_state)
   {
      if (intr->intrinsic != nir_intrinsic_store_output)
            nir_io_semantics io = nir_intrinsic_io_semantics(intr);
   if (io.location != VARYING_SLOT_POS)
            nir_def *src = intr->src[0].ssa;
   unsigned write_mask = nir_intrinsic_write_mask(intr);
   if (src->num_components == 4 && write_mask == 0xf)
            b->cursor = nir_before_instr(&intr->instr);
   unsigned first_comp = nir_intrinsic_component(intr);
   nir_def *channels[4] = { NULL, NULL, NULL, NULL };
   assert(first_comp + src->num_components <= ARRAY_SIZE(channels));
   for (unsigned i = 0; i < src->num_components; ++i)
      if (write_mask & (1 << i))
      for (unsigned i = 0; i < 4; ++i)
      if (!channels[i])
         intr->num_components = 4;
   nir_src_rewrite(&intr->src[0], nir_vec(b, channels, 4));
   nir_intrinsic_set_component(intr, 0);
   nir_intrinsic_set_write_mask(intr, 0xf);
      }
      bool
   dxil_nir_ensure_position_writes(nir_shader *s)
   {
      if (s->info.stage != MESA_SHADER_VERTEX &&
      s->info.stage != MESA_SHADER_GEOMETRY &&
   s->info.stage != MESA_SHADER_TESS_EVAL)
      if ((s->info.outputs_written & VARYING_BIT_POS) == 0)
            return nir_shader_intrinsics_pass(s, update_writes,
            }
      static bool
   is_sample_pos(const nir_instr *instr, const void *_data)
   {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
      }
      static nir_def *
   lower_sample_pos(nir_builder *b, nir_instr *instr, void *_data)
   {
         }
      bool
   dxil_nir_lower_sample_pos(nir_shader *s)
   {
         }
      static bool
   lower_subgroup_id(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_subgroup_id)
            b->cursor = nir_before_impl(b->impl);
   if (b->shader->info.workgroup_size[1] == 1 &&
      b->shader->info.workgroup_size[2] == 1) {
   /* When using Nx1x1 groups, use a simple stable algorithm
   * which is almost guaranteed to be correct. */
   nir_def *subgroup_id = nir_udiv(b, nir_load_local_invocation_index(b), nir_load_subgroup_size(b));
   nir_def_rewrite_uses(&intr->def, subgroup_id);
               nir_def **subgroup_id = (nir_def **)data;
   if (*subgroup_id == NULL) {
      nir_variable *subgroup_id_counter = nir_variable_create(b->shader, nir_var_mem_shared, glsl_uint_type(), "dxil_SubgroupID_counter");
   nir_variable *subgroup_id_local = nir_local_variable_create(b->impl, glsl_uint_type(), "dxil_SubgroupID_local");
            nir_deref_instr *counter_deref = nir_build_deref_var(b, subgroup_id_counter);
   nir_def *tid = nir_load_local_invocation_index(b);
   nir_if *nif = nir_push_if(b, nir_ieq_imm(b, tid, 0));
   nir_store_deref(b, counter_deref, nir_imm_int(b, 0), 1);
            nir_barrier(b,
                              nif = nir_push_if(b, nir_elect(b, 1));
   nir_def *subgroup_id_first_thread = nir_deref_atomic(b, 32, &counter_deref->def, nir_imm_int(b, 1),
         nir_store_var(b, subgroup_id_local, subgroup_id_first_thread, 1);
            nir_def *subgroup_id_loaded = nir_load_var(b, subgroup_id_local);
      }
   nir_def_rewrite_uses(&intr->def, *subgroup_id);
      }
      bool
   dxil_nir_lower_subgroup_id(nir_shader *s)
   {
      nir_def *subgroup_id = NULL;
   return nir_shader_intrinsics_pass(s, lower_subgroup_id, nir_metadata_none,
      }
      static bool
   lower_num_subgroups(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_num_subgroups)
            b->cursor = nir_before_instr(&intr->instr);
   nir_def *subgroup_size = nir_load_subgroup_size(b);
   nir_def *size_minus_one = nir_iadd_imm(b, subgroup_size, -1);
   nir_def *workgroup_size_vec = nir_load_workgroup_size(b);
   nir_def *workgroup_size = nir_imul(b, nir_channel(b, workgroup_size_vec, 0),
               nir_def *ret = nir_idiv(b, nir_iadd(b, workgroup_size, size_minus_one), subgroup_size);
   nir_def_rewrite_uses(&intr->def, ret);
      }
      bool
   dxil_nir_lower_num_subgroups(nir_shader *s)
   {
      return nir_shader_intrinsics_pass(s, lower_num_subgroups,
                  }
         static const struct glsl_type *
   get_cast_type(unsigned bit_size)
   {
      switch (bit_size) {
   case 64:
         case 32:
         case 16:
         case 8:
         }
      }
      static void
   split_unaligned_load(nir_builder *b, nir_intrinsic_instr *intrin, unsigned alignment)
   {
      enum gl_access_qualifier access = nir_intrinsic_access(intrin);
   nir_def *srcs[NIR_MAX_VEC_COMPONENTS * NIR_MAX_VEC_COMPONENTS * sizeof(int64_t) / 8];
   unsigned comp_size = intrin->def.bit_size / 8;
                              const struct glsl_type *cast_type = get_cast_type(alignment * 8);
            unsigned num_loads = DIV_ROUND_UP(comp_size * num_comps, alignment);
   for (unsigned i = 0; i < num_loads; ++i) {
      nir_deref_instr *elem = nir_build_deref_ptr_as_array(b, cast, nir_imm_intN_t(b, i, cast->def.bit_size));
               nir_def *new_dest = nir_extract_bits(b, srcs, num_loads, 0, num_comps, intrin->def.bit_size);
   nir_def_rewrite_uses(&intrin->def, new_dest);
      }
      static void
   split_unaligned_store(nir_builder *b, nir_intrinsic_instr *intrin, unsigned alignment)
   {
               nir_def *value = intrin->src[1].ssa;
   unsigned comp_size = value->bit_size / 8;
                              const struct glsl_type *cast_type = get_cast_type(alignment * 8);
            unsigned num_stores = DIV_ROUND_UP(comp_size * num_comps, alignment);
   for (unsigned i = 0; i < num_stores; ++i) {
      nir_def *substore_val = nir_extract_bits(b, &value, 1, i * alignment * 8, 1, alignment * 8);
   nir_deref_instr *elem = nir_build_deref_ptr_as_array(b, cast, nir_imm_intN_t(b, i, cast->def.bit_size));
                  }
      bool
   dxil_nir_split_unaligned_loads_stores(nir_shader *shader, nir_variable_mode modes)
   {
               nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
      intrin->intrinsic != nir_intrinsic_store_deref)
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                                          /* We can load anything at 4-byte alignment, except for
   * UBOs (AKA CBs where the granularity is 16 bytes).
   */
   unsigned req_align = (nir_deref_mode_is_one_of(deref, nir_var_mem_ubo | nir_var_mem_push_const) ? 16 : 4);
                  nir_def *val;
   if (intrin->intrinsic == nir_intrinsic_load_deref) {
         } else {
                  unsigned scalar_byte_size = glsl_type_is_boolean(deref->type) ? 4 : glsl_get_bit_size(deref->type) / 8;
   unsigned num_components =
      /* If the vector stride is larger than the scalar size, lower_explicit_io will
   * turn this into multiple scalar loads anyway, so we don't have to split it here. */
                                    if (intrin->intrinsic == nir_intrinsic_load_deref)
         else
                              }
      static void
   lower_inclusive_to_exclusive(nir_builder *b, nir_intrinsic_instr *intr)
   {
               nir_op op = nir_intrinsic_reduction_op(intr);
   intr->intrinsic = nir_intrinsic_exclusive_scan;
            nir_def *final_val = nir_build_alu2(b, nir_intrinsic_reduction_op(intr),
            }
      static bool
   lower_subgroup_scan(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_exclusive_scan:
   case nir_intrinsic_inclusive_scan:
      switch ((nir_op)nir_intrinsic_reduction_op(intr)) {
   case nir_op_iadd:
   case nir_op_fadd:
   case nir_op_imul:
   case nir_op_fmul:
      if (intr->intrinsic == nir_intrinsic_exclusive_scan)
         lower_inclusive_to_exclusive(b, intr);
      default:
         }
      default:
                  b->cursor = nir_before_instr(&intr->instr);
   nir_op op = nir_intrinsic_reduction_op(intr);
   nir_def *subgroup_id = nir_load_subgroup_invocation(b);
   nir_def *active_threads = nir_ballot(b, 4, 32, nir_imm_true(b));
   nir_def *base_value;
   uint32_t bit_size = intr->def.bit_size;
   if (op == nir_op_iand || op == nir_op_umin)
         else if (op == nir_op_imin)
         else if (op == nir_op_imax)
         else if (op == nir_op_fmax)
         else if (op == nir_op_fmin)
         else
            nir_variable *loop_counter_var = nir_local_variable_create(b->impl, glsl_uint_type(), "subgroup_loop_counter");
   nir_variable *result_var = nir_local_variable_create(b->impl,
                     nir_store_var(b, loop_counter_var, nir_imm_int(b, 0), 1);
   nir_store_var(b, result_var, base_value, 1);
   nir_loop *loop = nir_push_loop(b);
   nir_def *loop_counter = nir_load_var(b, loop_counter_var);
   nir_if *nif = nir_push_if(b, intr->intrinsic == nir_intrinsic_inclusive_scan ?
      nir_ige(b, subgroup_id, loop_counter) :
      nir_if *if_active_thread = nir_push_if(b, nir_ballot_bitfield_extract(b, 32, active_threads, loop_counter));
   nir_def *result = nir_build_alu2(b, op,
               nir_store_var(b, result_var, result, 1);
   nir_pop_if(b, if_active_thread);
   nir_store_var(b, loop_counter_var, nir_iadd_imm(b, loop_counter, 1), 1);
   nir_jump(b, nir_jump_continue);
   nir_pop_if(b, nif);
   nir_jump(b, nir_jump_break);
            result = nir_load_var(b, result_var);
   nir_def_rewrite_uses(&intr->def, result);
      }
      bool
   dxil_nir_lower_unsupported_subgroup_scan(nir_shader *s)
   {
      bool ret = nir_shader_intrinsics_pass(s, lower_subgroup_scan,
         if (ret) {
      /* Lower the ballot bitfield tests */
   nir_lower_subgroups_options options = { .ballot_bit_size = 32, .ballot_components = 4 };
      }
      }
      static bool
   lower_load_face(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_front_face)
                     nir_variable *var = data;
            nir_def_rewrite_uses(&intr->def, load);
   nir_instr_remove(&intr->instr);
      }
      bool
   dxil_nir_forward_front_face(nir_shader *nir)
   {
               nir_variable *var = nir_variable_create(nir, nir_var_shader_in,
               var->data.location = VARYING_SLOT_VAR12;
            return nir_shader_intrinsics_pass(nir, lower_load_face,
            }
      static bool
   move_consts(nir_builder *b, nir_instr *instr, void *data)
   {
      bool progress = false;
   switch (instr->type) {
   case nir_instr_type_load_const: {
      /* Sink load_const to their uses if there's multiple */
   nir_load_const_instr *load_const = nir_instr_as_load_const(instr);
   if (!list_is_singular(&load_const->def.uses)) {
      nir_foreach_use_safe(src, &load_const->def) {
      b->cursor = nir_before_src(src);
   nir_load_const_instr *new_load = nir_load_const_instr_create(b->shader,
               memcpy(new_load->value, load_const->value, sizeof(load_const->value[0]) * load_const->def.num_components);
   nir_builder_instr_insert(b, &new_load->instr);
   nir_src_rewrite(src, &new_load->def);
         }
      }
   default:
            }
      /* Sink all consts so that they have only have a single use.
   * The DXIL backend will already de-dupe the constants to the
   * same dxil_value if they have the same type, but this allows a single constant
   * to have different types without bitcasts. */
   bool
   dxil_nir_move_consts(nir_shader *s)
   {
      return nir_shader_instructions_pass(s, move_consts,
            }
      static void
   clear_pass_flags(nir_function_impl *impl)
   {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
               }
      static bool
   add_def_to_worklist(nir_def *def, void *state)
   {
      nir_foreach_use_including_if(src, def) {
      if (nir_src_is_if(src)) {
      nir_if *nif = nir_src_parent_if(src);
   nir_foreach_block_in_cf_node(block, &nif->cf_node) {
      nir_foreach_instr(instr, block)
         } else
      }
      }
      static bool
   set_input_bits(struct dxil_module *mod, nir_intrinsic_instr *intr, BITSET_WORD *input_bits, uint32_t ***tables, const uint32_t **table_sizes)
   {
      if (intr->intrinsic == nir_intrinsic_load_view_index) {
      BITSET_SET(input_bits, 0);
      }
      bool any_bits_set = false;
   nir_src *row_src = intr->intrinsic == nir_intrinsic_load_per_vertex_input ? &intr->src[1] : &intr->src[0];
   bool is_patch_constant = mod->shader_kind == DXIL_DOMAIN_SHADER && intr->intrinsic == nir_intrinsic_load_input;
   const struct dxil_signature_record *sig_rec = is_patch_constant ?
      &mod->patch_consts[nir_intrinsic_base(intr)] :
      if (is_patch_constant) {
      /* Redirect to the second I/O table */
   *tables = *tables + 1;
      }
   for (uint32_t component = 0; component < intr->num_components; ++component) {
      uint32_t base_element = 0;
   uint32_t num_elements = sig_rec->num_elements;
   if (nir_src_is_const(*row_src)) {
      base_element = (uint32_t)nir_src_as_uint(*row_src);
      }
   for (uint32_t element = 0; element < num_elements; ++element) {
      uint32_t row = sig_rec->elements[element + base_element].reg;
   if (row == 0xffffffff)
         BITSET_SET(input_bits, row * 4 + component + nir_intrinsic_component(intr));
         }
      }
      static bool
   set_output_bits(struct dxil_module *mod, nir_intrinsic_instr *intr, BITSET_WORD *input_bits, uint32_t **tables, const uint32_t *table_sizes)
   {
      bool any_bits_set = false;
   nir_src *row_src = intr->intrinsic == nir_intrinsic_store_per_vertex_output ? &intr->src[2] : &intr->src[1];
   bool is_patch_constant = mod->shader_kind == DXIL_HULL_SHADER && intr->intrinsic == nir_intrinsic_store_output;
   const struct dxil_signature_record *sig_rec = is_patch_constant ?
      &mod->patch_consts[nir_intrinsic_base(intr)] :
      for (uint32_t component = 0; component < intr->num_components; ++component) {
      uint32_t base_element = 0;
   uint32_t num_elements = sig_rec->num_elements;
   if (nir_src_is_const(*row_src)) {
      base_element = (uint32_t)nir_src_as_uint(*row_src);
      }
   for (uint32_t element = 0; element < num_elements; ++element) {
      uint32_t row = sig_rec->elements[element + base_element].reg;
   if (row == 0xffffffff)
         uint32_t stream = sig_rec->elements[element + base_element].stream;
   uint32_t table_idx = is_patch_constant ? 1 : stream;
   uint32_t *table = tables[table_idx];
   uint32_t output_component = component + nir_intrinsic_component(intr);
   uint32_t input_component;
   BITSET_FOREACH_SET(input_component, input_bits, 32 * 4) {
      uint32_t *table_for_input_component = table + table_sizes[table_idx] * input_component;
   BITSET_SET(table_for_input_component, row * 4 + output_component);
            }
      }
      static bool
   propagate_input_to_output_dependencies(struct dxil_module *mod, nir_intrinsic_instr *load_intr, uint32_t **tables, const uint32_t *table_sizes)
   {
      /* Which input components are being loaded by this instruction */
   BITSET_DECLARE(input_bits, 32 * 4) = { 0 };
   if (!set_input_bits(mod, load_intr, input_bits, &tables, &table_sizes))
            nir_instr_worklist *worklist = nir_instr_worklist_create();
   nir_instr_worklist_push_tail(worklist, &load_intr->instr);
   bool any_bits_set = false;
   nir_foreach_instr_in_worklist(instr, worklist) {
      if (instr->pass_flags)
            instr->pass_flags = 1;
   nir_foreach_def(instr, add_def_to_worklist, worklist);
   switch (instr->type) {
   case nir_instr_type_jump: {
      nir_jump_instr *jump = nir_instr_as_jump(instr);
   switch (jump->type) {
   case nir_jump_break:
   case nir_jump_continue: {
      nir_cf_node *parent = &instr->block->cf_node;
   while (parent->type != nir_cf_node_loop)
         nir_foreach_block_in_cf_node(block, parent)
      nir_foreach_instr(i, block)
      }
      default:
         }
      }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      any_bits_set |= set_output_bits(mod, intr, input_bits, tables, table_sizes);
   break;
      default:
         }
      }
   default:
                     nir_instr_worklist_destroy(worklist);
      }
      /* For every input load, compute the set of output stores that it can contribute to.
   * If it contributes to a store to memory, If it's used for control flow, then any
   * instruction in the CFG that it impacts is considered to contribute.
   * Ideally, we should also handle stores to outputs/memory and then loads from that
   * output/memory, but this is non-trivial and unclear how much impact that would have. */
   bool
   dxil_nir_analyze_io_dependencies(struct dxil_module *mod, nir_shader *s)
   {
      bool any_outputs = false;
   for (uint32_t i = 0; i < 4; ++i)
         if (mod->shader_kind == DXIL_HULL_SHADER)
         if (!any_outputs)
            bool any_bits_set = false;
   nir_foreach_function(func, s) {
      assert(func->impl);
   /* Hull shaders have a patch constant function */
            /* Pass 1: input/view ID -> output dependencies */
   nir_foreach_block(block, func->impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   uint32_t **tables = mod->io_dependency_table;
   const uint32_t *table_sizes = mod->dependency_table_dwords_per_input;
   switch (intr->intrinsic) {
   case nir_intrinsic_load_view_index:
      tables = mod->viewid_dependency_table;
      case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
         default:
                  clear_pass_flags(func->impl);
                  /* Pass 2: output -> output dependencies */
      }
      }
      static enum pipe_format
   get_format_for_var(unsigned num_comps, enum glsl_base_type sampled_type)
   {
      switch (sampled_type) {
   case GLSL_TYPE_INT:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_INT16:
      switch (num_comps) {
   case 1: return PIPE_FORMAT_R32_SINT;
   case 2: return PIPE_FORMAT_R32G32_SINT;
   case 3: return PIPE_FORMAT_R32G32B32_SINT;
   case 4: return PIPE_FORMAT_R32G32B32A32_SINT;
   default: unreachable("Invalid num_comps");
      case GLSL_TYPE_UINT:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_UINT16:
      switch (num_comps) {
   case 1: return PIPE_FORMAT_R32_UINT;
   case 2: return PIPE_FORMAT_R32G32_UINT;
   case 3: return PIPE_FORMAT_R32G32B32_UINT;
   case 4: return PIPE_FORMAT_R32G32B32A32_UINT;
   default: unreachable("Invalid num_comps");
      case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
      switch (num_comps) {
   case 1: return PIPE_FORMAT_R32_FLOAT;
   case 2: return PIPE_FORMAT_R32G32_FLOAT;
   case 3: return PIPE_FORMAT_R32G32B32_FLOAT;
   case 4: return PIPE_FORMAT_R32G32B32A32_FLOAT;
   default: unreachable("Invalid num_comps");
      default: unreachable("Invalid sampler return type");
      }
      static unsigned
   aoa_size(const struct glsl_type *type)
   {
         }
      static bool
   guess_image_format_for_var(nir_shader *s, nir_variable *var)
   {
      const struct glsl_type *base_type = glsl_without_array(var->type);
   if (!glsl_type_is_image(base_type))
         if (var->data.image.format != PIPE_FORMAT_NONE)
            nir_foreach_function_impl(impl, s) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
      if (nir_intrinsic_get_var(intr, 0) != var)
            case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap: {
      unsigned binding = nir_src_as_uint(intr->src[0]);
   if (binding < var->data.binding ||
      binding >= var->data.binding + aoa_size(var->type))
      break;
      default:
                        switch (intr->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_store:
      /* Increase unknown formats up to 4 components if a 4-component accessor is used */
   if (intr->num_components > util_format_get_nr_components(var->data.image.format))
            default:
      /* If an atomic is used, the image format must be 1-component; return immediately */
   var->data.image.format = get_format_for_var(1, glsl_get_sampler_result_type(base_type));
               }
   /* Dunno what it is, assume 4-component */
   if (var->data.image.format == PIPE_FORMAT_NONE)
            }
      static bool
   update_intrinsic_formats(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (!nir_intrinsic_has_format(intr))
         nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   if (deref) {
      nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var)
                     if (!nir_intrinsic_has_range_base(intr))
            unsigned binding = nir_src_as_uint(intr->src[0]);
   nir_foreach_variable_with_modes(var, b->shader, nir_var_image) {
      if (var->data.binding <= binding &&
      var->data.binding + aoa_size(var->type) > binding) {
   nir_intrinsic_set_format(intr, var->data.image.format);
         }
      }
      bool
   dxil_nir_guess_image_formats(nir_shader *s)
   {
      bool progress = false;
   nir_foreach_variable_with_modes(var, s, nir_var_image) {
         }
   nir_shader_intrinsics_pass(s, update_intrinsic_formats, nir_metadata_all,
            }
