   /*
   * Copyright 2023 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "glsl_types.h"
   #include "nir.h"
   #include "nir_types.h"
   #include "vtn_private.h"
      static enum glsl_cmat_use
   vtn_cooperative_matrix_use_to_glsl(SpvCooperativeMatrixUse use)
   {
      switch (use) {
   case SpvCooperativeMatrixUseMatrixAKHR:
         case SpvCooperativeMatrixUseMatrixBKHR:
         case SpvCooperativeMatrixUseMatrixAccumulatorKHR:
         default:
            }
      void
   vtn_handle_cooperative_type(struct vtn_builder *b, struct vtn_value *val,
         {
                                 const mesa_scope scope = vtn_translate_scope(b, vtn_constant_uint(b, w[3]));
   const uint32_t rows = vtn_constant_uint(b, w[4]);
            vtn_assert(rows < 256);
                     val->type->base_type = vtn_base_type_cooperative_matrix;
   vtn_fail_if(!glsl_type_is_numeric(component_type->type),
                  val->type->desc.element_type = glsl_get_base_type(component_type->type);
   val->type->desc.scope = scope;
   val->type->desc.rows = rows;
   val->type->desc.cols = cols;
            val->type->type = glsl_cmat_type(&val->type->desc);
      }
      static enum glsl_matrix_layout
   vtn_matrix_layout_to_glsl(SpvCooperativeMatrixLayout layout)
   {
      switch (layout) {
   case SpvCooperativeMatrixLayoutRowMajorKHR:
         case SpvCooperativeMatrixLayoutColumnMajorKHR:
         default:
            }
      nir_deref_instr *
   vtn_create_cmat_temporary(struct vtn_builder *b, const struct glsl_type *t, const char *name)
   {
      nir_variable *var = nir_local_variable_create(b->nb.impl, t, name);
      }
      static nir_deref_instr *
   vtn_get_cmat_deref(struct vtn_builder *b, uint32_t value_id)
   {
      nir_deref_instr *deref = vtn_get_deref_for_id(b, value_id);
   vtn_assert(glsl_type_is_cmat(deref->type));
      }
      void
   vtn_handle_cooperative_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpCooperativeMatrixLoadKHR: {
      struct vtn_value *src_val = vtn_value(b, w[3], vtn_value_type_pointer);
   struct vtn_pointer *src = vtn_value_to_pointer(b, src_val);
            const SpvCooperativeMatrixLayout layout = vtn_constant_uint(b, w[4]);
            SpvMemoryAccessMask access = SpvMemoryAccessMaskNone;
   if (count > 6) {
      unsigned idx = 6, alignment;
   SpvScope scope;
   vtn_get_mem_operands(b, w, count, &idx, &access, &alignment, NULL, &scope);
               nir_deref_instr *dst = vtn_create_cmat_temporary(b, dst_type->type, "cmat_bitcast");
   nir_cmat_load(&b->nb, &dst->def, vtn_pointer_to_ssa(b, src), stride,
         vtn_push_var_ssa(b, w[2], dst->var);
               case SpvOpCooperativeMatrixStoreKHR: {
      struct vtn_value *dest_val = vtn_value(b, w[1], vtn_value_type_pointer);
            const SpvCooperativeMatrixLayout layout = vtn_constant_uint(b, w[3]);
            SpvMemoryAccessMask access = SpvMemoryAccessMaskNone;
   if (count > 5) {
      unsigned idx = 5, alignment;
   SpvScope scope;
   vtn_get_mem_operands(b, w, count, &idx, &access, &alignment, &scope, NULL);
               nir_deref_instr *src = vtn_get_cmat_deref(b, w[2]);
   nir_cmat_store(&b->nb, vtn_pointer_to_ssa(b, dest), &src->def, stride,
                     case SpvOpCooperativeMatrixLengthKHR: {
      struct vtn_type *type = vtn_get_type(b, w[3]);
   nir_def *def = nir_cmat_length(&b->nb, .cmat_desc = type->desc);
   vtn_push_nir_ssa(b, w[2], def);
               case SpvOpCooperativeMatrixMulAddKHR: {
      nir_deref_instr *mat_a = vtn_get_cmat_deref(b, w[3]);
   nir_deref_instr *mat_b = vtn_get_cmat_deref(b, w[4]);
            const uint32_t operands = count > 6 ? w[6] : 0;
   const bool saturate = operands & SpvCooperativeMatrixOperandsSaturatingAccumulationKHRMask;
   const unsigned signed_mask = operands & (SpvCooperativeMatrixOperandsMatrixASignedComponentsKHRMask |
                        STATIC_ASSERT((unsigned)SpvCooperativeMatrixOperandsMatrixASignedComponentsKHRMask == NIR_CMAT_A_SIGNED);
   STATIC_ASSERT((unsigned)SpvCooperativeMatrixOperandsMatrixBSignedComponentsKHRMask == NIR_CMAT_B_SIGNED);
   STATIC_ASSERT((unsigned)SpvCooperativeMatrixOperandsMatrixCSignedComponentsKHRMask == NIR_CMAT_C_SIGNED);
            struct vtn_type *dst_type = vtn_get_type(b, w[1]);
            nir_cmat_muladd(&b->nb, &dst->def, &mat_a->def, &mat_b->def, &mat_c->def,
                  vtn_push_var_ssa(b, w[2], dst->var);
               case SpvOpBitcast: {
      struct vtn_type *dst_type = vtn_get_type(b, w[1]);
   vtn_assert(dst_type->base_type == vtn_base_type_cooperative_matrix);
            nir_deref_instr *dst = vtn_create_cmat_temporary(b, dst_type->type, "cmat_bitcast");
   nir_cmat_bitcast(&b->nb, &dst->def, &src->def);
   vtn_push_var_ssa(b, w[2], dst->var);
               default:
            }
      void
   vtn_handle_cooperative_alu(struct vtn_builder *b, struct vtn_value *dest_val,
               {
                  switch (opcode) {
   case SpvOpConvertFToU:
   case SpvOpConvertFToS:
   case SpvOpConvertSToF:
   case SpvOpConvertUToF:
   case SpvOpUConvert:
   case SpvOpSConvert:
   case SpvOpFConvert:
   case SpvOpFNegate:
   case SpvOpSNegate: {
                                    bool ignored = false;
                  nir_deref_instr *dst = vtn_create_cmat_temporary(b, dst_type->type, "cmat_unary");
   nir_cmat_unary_op(&b->nb, &dst->def, &src->def,
         vtn_push_var_ssa(b, w[2], dst->var);
               case SpvOpFAdd:
   case SpvOpFSub:
   case SpvOpFMul:
   case SpvOpFDiv:
   case SpvOpIAdd:
   case SpvOpISub:
   case SpvOpIMul:
   case SpvOpSDiv:
   case SpvOpUDiv: {
                     struct vtn_type *dst_type = vtn_get_type(b, w[1]);
                  nir_deref_instr *dst = vtn_create_cmat_temporary(b, dst_type->type, "cmat_binary");
   nir_cmat_binary_op(&b->nb, &dst->def, &mat_a->def, &mat_b->def,
         vtn_push_var_ssa(b, w[2], dst->var);
               case SpvOpMatrixTimesScalar: {
                     struct vtn_ssa_value *scalar_val = vtn_ssa_value(b, w[4]);
                  nir_deref_instr *dst = vtn_create_cmat_temporary(b, dst_type->type, "cmat_times_scalar");
   nir_cmat_scalar_op(&b->nb, &dst->def, &mat->def, scalar_val->def,
         vtn_push_var_ssa(b, w[2], dst->var);
               default:
         }
      struct vtn_ssa_value *
   vtn_cooperative_matrix_extract(struct vtn_builder *b, struct vtn_ssa_value *mat,
         {
      vtn_assert(glsl_type_is_cmat(mat->type));
            vtn_assert(num_indices == 1);
            const struct glsl_type *element_type = glsl_get_cmat_element(mat->type);
   struct vtn_ssa_value *ret = vtn_create_ssa_value(b, element_type);
   ret->def = nir_cmat_extract(&b->nb, glsl_get_bit_size(element_type), &mat_deref->def, index);
      }
      struct vtn_ssa_value *
   vtn_cooperative_matrix_insert(struct vtn_builder *b, struct vtn_ssa_value *mat,
               {
      vtn_assert(glsl_type_is_cmat(mat->type));
            vtn_assert(num_indices == 1);
            nir_deref_instr *dst = vtn_create_cmat_temporary(b, mat_deref->type, "cmat_insert");
            struct vtn_ssa_value *ret = vtn_create_ssa_value(b, dst->type);
   vtn_set_ssa_value_var(b, ret, dst->var);
      }
