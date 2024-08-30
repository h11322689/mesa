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
   * \file ir_constant_expression.cpp
   * Evaluate and process constant valued expressions
   *
   * In GLSL, constant valued expressions are used in several places.  These
   * must be processed and evaluated very early in the compilation process.
   *
   *    * Sizes of arrays
   *    * Initializers for uniforms
   *    * Initializers for \c const variables
   */
      #include <math.h>
   #include "util/rounding.h" /* for _mesa_roundeven */
   #include "util/half_float.h"
   #include "ir.h"
   #include "compiler/glsl_types.h"
   #include "util/hash_table.h"
   #include "util/u_math.h"
      static float
   dot_f(ir_constant *op0, ir_constant *op1)
   {
               float result = 0;
   for (unsigned c = 0; c < op0->type->components(); c++)
               }
      static double
   dot_d(ir_constant *op0, ir_constant *op1)
   {
               double result = 0;
   for (unsigned c = 0; c < op0->type->components(); c++)
               }
      /* This method is the only one supported by gcc.  Unions in particular
   * are iffy, and read-through-converted-pointer is killed by strict
   * aliasing.  OTOH, the compiler sees through the memcpy, so the
   * resulting asm is reasonable.
   */
   static float
   bitcast_u2f(unsigned int u)
   {
      static_assert(sizeof(float) == sizeof(unsigned int),
         float f;
   memcpy(&f, &u, sizeof(f));
      }
      static unsigned int
   bitcast_f2u(float f)
   {
      static_assert(sizeof(float) == sizeof(unsigned int),
         unsigned int u;
   memcpy(&u, &f, sizeof(f));
      }
      static double
   bitcast_u642d(uint64_t u)
   {
      static_assert(sizeof(double) == sizeof(uint64_t),
         double d;
   memcpy(&d, &u, sizeof(d));
      }
      static double
   bitcast_i642d(int64_t i)
   {
      static_assert(sizeof(double) == sizeof(int64_t),
         double d;
   memcpy(&d, &i, sizeof(d));
      }
      static uint64_t
   bitcast_d2u64(double d)
   {
      static_assert(sizeof(double) == sizeof(uint64_t),
         uint64_t u;
   memcpy(&u, &d, sizeof(d));
      }
      static int64_t
   bitcast_d2i64(double d)
   {
      static_assert(sizeof(double) == sizeof(int64_t),
         int64_t i;
   memcpy(&i, &d, sizeof(d));
      }
      /**
   * Evaluate one component of a floating-point 4x8 unpacking function.
   */
   typedef uint8_t
   (*pack_1x8_func_t)(float);
      /**
   * Evaluate one component of a floating-point 2x16 unpacking function.
   */
   typedef uint16_t
   (*pack_1x16_func_t)(float);
      /**
   * Evaluate one component of a floating-point 4x8 unpacking function.
   */
   typedef float
   (*unpack_1x8_func_t)(uint8_t);
      /**
   * Evaluate one component of a floating-point 2x16 unpacking function.
   */
   typedef float
   (*unpack_1x16_func_t)(uint16_t);
      /**
   * Evaluate a 2x16 floating-point packing function.
   */
   static uint32_t
   pack_2x16(pack_1x16_func_t pack_1x16,
         {
      /* From section 8.4 of the GLSL ES 3.00 spec:
   *
   *    packSnorm2x16
   *    -------------
   *    The first component of the vector will be written to the least
   *    significant bits of the output; the last component will be written to
   *    the most significant bits.
   *
   * The specifications for the other packing functions contain similar
   * language.
   */
   uint32_t u = 0;
   u |= ((uint32_t) pack_1x16(x) << 0);
   u |= ((uint32_t) pack_1x16(y) << 16);
      }
      /**
   * Evaluate a 4x8 floating-point packing function.
   */
   static uint32_t
   pack_4x8(pack_1x8_func_t pack_1x8,
         {
      /* From section 8.4 of the GLSL 4.30 spec:
   *
   *    packSnorm4x8
   *    ------------
   *    The first component of the vector will be written to the least
   *    significant bits of the output; the last component will be written to
   *    the most significant bits.
   *
   * The specifications for the other packing functions contain similar
   * language.
   */
   uint32_t u = 0;
   u |= ((uint32_t) pack_1x8(x) << 0);
   u |= ((uint32_t) pack_1x8(y) << 8);
   u |= ((uint32_t) pack_1x8(z) << 16);
   u |= ((uint32_t) pack_1x8(w) << 24);
      }
      /**
   * Evaluate a 2x16 floating-point unpacking function.
   */
   static void
   unpack_2x16(unpack_1x16_func_t unpack_1x16,
               {
      /* From section 8.4 of the GLSL ES 3.00 spec:
   *
   *    unpackSnorm2x16
   *    ---------------
   *    The first component of the returned vector will be extracted from
   *    the least significant bits of the input; the last component will be
   *    extracted from the most significant bits.
   *
   * The specifications for the other unpacking functions contain similar
   * language.
   */
   *x = unpack_1x16((uint16_t) (u & 0xffff));
      }
      /**
   * Evaluate a 4x8 floating-point unpacking function.
   */
   static void
   unpack_4x8(unpack_1x8_func_t unpack_1x8, uint32_t u,
         {
      /* From section 8.4 of the GLSL 4.30 spec:
   *
   *    unpackSnorm4x8
   *    --------------
   *    The first component of the returned vector will be extracted from
   *    the least significant bits of the input; the last component will be
   *    extracted from the most significant bits.
   *
   * The specifications for the other unpacking functions contain similar
   * language.
   */
   *x = unpack_1x8((uint8_t) (u & 0xff));
   *y = unpack_1x8((uint8_t) (u >> 8));
   *z = unpack_1x8((uint8_t) (u >> 16));
      }
      /**
   * Evaluate one component of packSnorm4x8.
   */
   static uint8_t
   pack_snorm_1x8(float x)
   {
      /* From section 8.4 of the GLSL 4.30 spec:
   *
   *    packSnorm4x8
   *    ------------
   *    The conversion for component c of v to fixed point is done as
   *    follows:
   *
   *      packSnorm4x8: round(clamp(c, -1, +1) * 127.0)
   */
   return (uint8_t)
      }
      /**
   * Evaluate one component of packSnorm2x16.
   */
   static uint16_t
   pack_snorm_1x16(float x)
   {
      /* From section 8.4 of the GLSL ES 3.00 spec:
   *
   *    packSnorm2x16
   *    -------------
   *    The conversion for component c of v to fixed point is done as
   *    follows:
   *
   *      packSnorm2x16: round(clamp(c, -1, +1) * 32767.0)
   */
   return (uint16_t)
      }
      /**
   * Evaluate one component of unpackSnorm4x8.
   */
   static float
   unpack_snorm_1x8(uint8_t u)
   {
      /* From section 8.4 of the GLSL 4.30 spec:
   *
   *    unpackSnorm4x8
   *    --------------
   *    The conversion for unpacked fixed-point value f to floating point is
   *    done as follows:
   *
   *       unpackSnorm4x8: clamp(f / 127.0, -1, +1)
   */
      }
      /**
   * Evaluate one component of unpackSnorm2x16.
   */
   static float
   unpack_snorm_1x16(uint16_t u)
   {
      /* From section 8.4 of the GLSL ES 3.00 spec:
   *
   *    unpackSnorm2x16
   *    ---------------
   *    The conversion for unpacked fixed-point value f to floating point is
   *    done as follows:
   *
   *       unpackSnorm2x16: clamp(f / 32767.0, -1, +1)
   */
      }
      /**
   * Evaluate one component packUnorm4x8.
   */
   static uint8_t
   pack_unorm_1x8(float x)
   {
      /* From section 8.4 of the GLSL 4.30 spec:
   *
   *    packUnorm4x8
   *    ------------
   *    The conversion for component c of v to fixed point is done as
   *    follows:
   *
   *       packUnorm4x8: round(clamp(c, 0, +1) * 255.0)
   */
      }
      /**
   * Evaluate one component packUnorm2x16.
   */
   static uint16_t
   pack_unorm_1x16(float x)
   {
      /* From section 8.4 of the GLSL ES 3.00 spec:
   *
   *    packUnorm2x16
   *    -------------
   *    The conversion for component c of v to fixed point is done as
   *    follows:
   *
   *       packUnorm2x16: round(clamp(c, 0, +1) * 65535.0)
   */
   return (uint16_t) (int)
      }
      /**
   * Evaluate one component of unpackUnorm4x8.
   */
   static float
   unpack_unorm_1x8(uint8_t u)
   {
      /* From section 8.4 of the GLSL 4.30 spec:
   *
   *    unpackUnorm4x8
   *    --------------
   *    The conversion for unpacked fixed-point value f to floating point is
   *    done as follows:
   *
   *       unpackUnorm4x8: f / 255.0
   */
      }
      /**
   * Evaluate one component of unpackUnorm2x16.
   */
   static float
   unpack_unorm_1x16(uint16_t u)
   {
      /* From section 8.4 of the GLSL ES 3.00 spec:
   *
   *    unpackUnorm2x16
   *    ---------------
   *    The conversion for unpacked fixed-point value f to floating point is
   *    done as follows:
   *
   *       unpackUnorm2x16: f / 65535.0
   */
      }
      /**
   * Evaluate one component of packHalf2x16.
   */
   static uint16_t
   pack_half_1x16(float x)
   {
         }
      /**
   * Evaluate one component of unpackHalf2x16.
   */
   static float
   unpack_half_1x16(uint16_t u)
   {
         }
      static int32_t
   iadd_saturate(int32_t a, int32_t b)
   {
         }
      static int64_t
   iadd64_saturate(int64_t a, int64_t b)
   {
      if (a < 0 && b < INT64_MIN - a)
            if (a > 0 && b > INT64_MAX - a)
               }
      static int32_t
   isub_saturate(int32_t a, int32_t b)
   {
         }
      static int64_t
   isub64_saturate(int64_t a, int64_t b)
   {
      if (b > 0 && a < INT64_MIN + b)
            if (b < 0 && a > INT64_MAX + b)
               }
      static uint64_t
   pack_2x32(uint32_t a, uint32_t b)
   {
      uint64_t v = a;
   v |= (uint64_t)b << 32;
      }
      static void
   unpack_2x32(uint64_t p, uint32_t *a, uint32_t *b)
   {
      *a = p & 0xffffffff;
      }
      /**
   * Get the constant that is ultimately referenced by an r-value, in a constant
   * expression evaluation context.
   *
   * The offset is used when the reference is to a specific column of a matrix.
   */
   static bool
   constant_referenced(const ir_dereference *deref,
               {
      store = NULL;
            if (variable_context == NULL)
            switch (deref->ir_type) {
   case ir_type_dereference_array: {
      const ir_dereference_array *const da =
            ir_constant *const index_c =
            if (!index_c || !index_c->type->is_scalar() ||
                  const int index = index_c->type->base_type == GLSL_TYPE_INT ?
                  ir_constant *substore;
            const ir_dereference *const deref = da->array->as_dereference();
   if (!deref)
            if (!constant_referenced(deref, variable_context, substore, suboffset))
            const glsl_type *const vt = da->array->type;
   if (vt->is_array()) {
      store = substore->get_array_element(index);
      } else if (vt->is_matrix()) {
      store = substore;
      } else if (vt->is_vector()) {
      store = substore;
                           case ir_type_dereference_record: {
      const ir_dereference_record *const dr =
            const ir_dereference *const deref = dr->record->as_dereference();
   if (!deref)
            ir_constant *substore;
            if (!constant_referenced(deref, variable_context, substore, suboffset))
            /* Since we're dropping it on the floor...
   */
            store = substore->get_record_field(dr->field_idx);
               case ir_type_dereference_variable: {
      const ir_dereference_variable *const dv =
            hash_entry *entry = _mesa_hash_table_search(variable_context, dv->var);
   if (entry)
                     default:
      assert(!"Should not get here.");
                  }
         ir_constant *
   ir_rvalue::constant_expression_value(void *, struct hash_table *)
   {
      assert(this->type->is_error());
      }
      static uint32_t
   bitfield_reverse(uint32_t v)
   {
      /* http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious */
   uint32_t r = v; // r will be reversed bits of v; first get LSB of v
            for (v >>= 1; v; v >>= 1) {
      r <<= 1;
   r |= v & 1;
      }
               }
      static int
   find_msb_uint(uint32_t v)
   {
               /* If v == 0, then the loop will terminate when count == 32.  In that case
   * 31-count will produce the -1 result required by GLSL findMSB().
   */
   while (((v & (1u << 31)) == 0) && count != 32) {
      count++;
                  }
      static int
   find_msb_int(int32_t v)
   {
      /* If v is signed, findMSB() returns the position of the most significant
   * zero bit.
   */
      }
      static float
   ldexpf_flush_subnormal(float x, int exp)
   {
               /* Flush subnormal values to zero. */
      }
      static double
   ldexp_flush_subnormal(double x, int exp)
   {
               /* Flush subnormal values to zero. */
      }
      static uint32_t
   bitfield_extract_uint(uint32_t value, int offset, int bits)
   {
      if (bits == 0)
         else if (offset < 0 || bits < 0)
         else if (offset + bits > 32)
         else {
      value <<= 32 - bits - offset;
   value >>= 32 - bits;
         }
      static int32_t
   bitfield_extract_int(int32_t value, int offset, int bits)
   {
      if (bits == 0)
         else if (offset < 0 || bits < 0)
         else if (offset + bits > 32)
         else {
      value <<= 32 - bits - offset;
   value >>= 32 - bits;
         }
      static uint32_t
   bitfield_insert(uint32_t base, uint32_t insert, int offset, int bits)
   {
      if (bits == 0)
         else if (offset < 0 || bits < 0)
         else if (offset + bits > 32)
         else {
               insert <<= offset;
   insert &= insert_mask;
                  }
      ir_constant *
   ir_expression::constant_expression_value(void *mem_ctx,
         {
               if (this->type->is_error())
            const glsl_type *return_type = this->type;
   ir_constant *op[ARRAY_SIZE(this->operands)] = { NULL, };
                     for (unsigned operand = 0; operand < this->num_operands; operand++) {
      op[operand] =
      this->operands[operand]->constant_expression_value(mem_ctx,
      if (!op[operand])
               for (unsigned operand = 0; operand < this->num_operands; operand++) {
      switch (op[operand]->type->base_type) {
   case GLSL_TYPE_FLOAT16: {
      const struct glsl_type *float_type =
         glsl_type::get_instance(GLSL_TYPE_FLOAT,
                        ir_constant_data f;
                  op[operand] = new(mem_ctx) ir_constant(float_type, &f);
      }
   case GLSL_TYPE_INT16: {
      const struct glsl_type *int_type =
      glsl_type::get_instance(GLSL_TYPE_INT,
                           ir_constant_data d;
                  op[operand] = new(mem_ctx) ir_constant(int_type, &d);
      }
   case GLSL_TYPE_UINT16: {
      const struct glsl_type *uint_type =
      glsl_type::get_instance(GLSL_TYPE_UINT,
                           ir_constant_data d;
                  op[operand] = new(mem_ctx) ir_constant(uint_type, &d);
      }
   default:
      /* nothing to do */
                  switch (return_type->base_type) {
   case GLSL_TYPE_FLOAT16:
      return_type = glsl_type::get_instance(GLSL_TYPE_FLOAT,
                              case GLSL_TYPE_INT16:
      return_type = glsl_type::get_instance(GLSL_TYPE_INT,
                              case GLSL_TYPE_UINT16:
      return_type = glsl_type::get_instance(GLSL_TYPE_UINT,
                              default:
      /* nothing to do */
               if (op[1] != NULL)
      switch (this->operation) {
   case ir_binop_lshift:
   case ir_binop_rshift:
   case ir_binop_ldexp:
   case ir_binop_interpolate_at_offset:
   case ir_binop_interpolate_at_sample:
   case ir_binop_vector_extract:
   case ir_triop_csel:
   case ir_triop_bitfield_extract:
            default:
      assert(op[0]->type->base_type == op[1]->type->base_type);
            bool op0_scalar = op[0]->type->is_scalar();
            /* When iterating over a vector or matrix's components, we want to increase
   * the loop counter.  However, for scalars, we want to stay at 0.
   */
   unsigned c0_inc = op0_scalar ? 0 : 1;
   unsigned c1_inc = op1_scalar ? 0 : 1;
   unsigned components;
   if (op1_scalar || !op[1]) {
         } else {
                  /* Handle array operations here, rather than below. */
   if (op[0]->type->is_array()) {
      assert(op[1] != NULL && op[1]->type->is_array());
   switch (this->operation) {
   case ir_binop_all_equal:
         case ir_binop_any_nequal:
         default:
         }
            #include "ir_expression_operation_constant.h"
         switch (type->base_type) {
   case GLSL_TYPE_FLOAT16: {
      ir_constant_data f;
   for (unsigned i = 0; i < ARRAY_SIZE(f.f16); i++)
               }
   case GLSL_TYPE_INT16: {
      ir_constant_data d;
   for (unsigned i = 0; i < ARRAY_SIZE(d.i16); i++)
               }
   case GLSL_TYPE_UINT16: {
      ir_constant_data d;
   for (unsigned i = 0; i < ARRAY_SIZE(d.u16); i++)
               }
   default:
            }
         ir_constant *
   ir_texture::constant_expression_value(void *, struct hash_table *)
   {
      /* texture lookups aren't constant expressions */
      }
         ir_constant *
   ir_swizzle::constant_expression_value(void *mem_ctx,
         {
               ir_constant *v = this->val->constant_expression_value(mem_ctx,
            if (v != NULL) {
               const unsigned swiz_idx[4] = {
                  for (unsigned i = 0; i < this->mask.num_components; i++) {
      switch (v->type->base_type) {
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16: data.u16[i] = v->value.u16[swiz_idx[i]]; break;
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:   data.u[i] = v->value.u[swiz_idx[i]]; break;
   case GLSL_TYPE_FLOAT: data.f[i] = v->value.f[swiz_idx[i]]; break;
   case GLSL_TYPE_FLOAT16: data.f16[i] = v->value.f16[swiz_idx[i]]; break;
   case GLSL_TYPE_BOOL:  data.b[i] = v->value.b[swiz_idx[i]]; break;
   case GLSL_TYPE_DOUBLE:data.d[i] = v->value.d[swiz_idx[i]]; break;
   case GLSL_TYPE_UINT64:data.u64[i] = v->value.u64[swiz_idx[i]]; break;
   case GLSL_TYPE_INT64: data.i64[i] = v->value.i64[swiz_idx[i]]; break;
   default:              assert(!"Should not get here."); break;
                  }
      }
         ir_constant *
   ir_dereference_variable::constant_expression_value(void *mem_ctx,
         {
      assert(var);
            /* Give priority to the context hashtable, if it exists */
   if (variable_context) {
               if(entry)
               /* The constant_value of a uniform variable is its initializer,
   * not the lifetime constant value of the uniform.
   */
   if (var->data.mode == ir_var_uniform)
            if (!var->constant_value)
               }
         ir_constant *
   ir_dereference_array::constant_expression_value(void *mem_ctx,
         {
               ir_constant *array = this->array->constant_expression_value(mem_ctx, variable_context);
            if ((array != NULL) && (idx != NULL)) {
      if (array->type->is_matrix()) {
      /* Array access of a matrix results in a vector.
                           /* Section 5.11 (Out-of-Bounds Accesses) of the GLSL 4.60 spec says:
   *
   *    In the subsections described above for array, vector, matrix and
   *    structure accesses, any out-of-bounds access produced undefined
   *    behavior....Out-of-bounds reads return undefined values, which
   *    include values from other variables of the active program or zero.
   */
                                 /* Offset in the constant matrix to the first element of the column
   * to be extracted.
                           switch (column_type->base_type) {
   case GLSL_TYPE_FLOAT16:
                           case GLSL_TYPE_FLOAT:
                           case GLSL_TYPE_DOUBLE:
                           default:
                     } else if (array->type->is_vector()) {
                  } else if (array->type->is_array()) {
      const unsigned index = idx->value.u[0];
         }
      }
         ir_constant *
   ir_dereference_record::constant_expression_value(void *mem_ctx,
         {
                           }
         ir_constant *
   ir_assignment::constant_expression_value(void *, struct hash_table *)
   {
      /* FINISHME: Handle CEs involving assignment (return RHS) */
      }
         ir_constant *
   ir_constant::constant_expression_value(void *, struct hash_table *)
   {
         }
         ir_constant *
   ir_call::constant_expression_value(void *mem_ctx, struct hash_table *variable_context)
   {
               return this->callee->constant_expression_value(mem_ctx,
            }
         bool ir_function_signature::constant_expression_evaluate_expression_list(void *mem_ctx,
                     {
               foreach_in_list(ir_instruction, inst, &body) {
                     case ir_type_variable: {
      ir_variable *var = inst->as_variable();
   _mesa_hash_table_insert(variable_context, var, ir_constant::zero(this, var->type));
                     case ir_type_assignment: {
      ir_assignment *asg = inst->as_assignment();
                                                               store->copy_masked_offset(value, offset, asg->write_mask);
                     case ir_type_return:
      assert (result);
   *result =
      inst->as_return()->value->constant_expression_value(mem_ctx,
                  case ir_type_call: {
               /* Just say no to void functions in constant expressions.  We
                                                if (!constant_referenced(call->return_deref, variable_context,
                                                store->copy_offset(value, offset);
                     case ir_type_if: {
               ir_constant *cond =
      iif->condition->constant_expression_value(mem_ctx,
                              *result = NULL;
   if (!constant_expression_evaluate_expression_list(mem_ctx, branch,
                        /* If there was a return in the branch chosen, drop out now. */
                                    default:
                     /* Reaching the end of the block is not an error condition */
   if (result)
               }
      ir_constant *
   ir_function_signature::constant_expression_value(void *mem_ctx,
               {
               const glsl_type *type = this->return_type;
   if (type == glsl_type::void_type)
            /* From the GLSL 1.20 spec, page 23:
   * "Function calls to user-defined functions (non-built-in functions)
   *  cannot be used to form constant expressions."
   */
   if (!this->is_builtin())
            /*
   * Of the builtin functions, only the texture lookups and the noise
   * ones must not be used in constant expressions.  Texture instructions
   * include special ir_texture opcodes which can't be constant-folded (see
   * ir_texture::constant_expression_value).  Noise functions, however, we
   * have to special case here.
   */
   if (strcmp(this->function_name(), "noise1") == 0 ||
      strcmp(this->function_name(), "noise2") == 0 ||
   strcmp(this->function_name(), "noise3") == 0 ||
   strcmp(this->function_name(), "noise4") == 0)
         /* Initialize the table of dereferencable names with the function
   * parameters.  Verify their const-ness on the way.
   *
   * We expect the correctness of the number of parameters to have
   * been checked earlier.
   */
            /* If "origin" is non-NULL, then the function body is there.  So we
   * have to use the variable objects from the object with the body,
   * but the parameter instanciation on the current object.
   */
            foreach_in_list(ir_rvalue, n, actual_parameters) {
      ir_constant *constant =
         if (constant == NULL) {
      _mesa_hash_table_destroy(deref_hash, NULL);
                  ir_variable *var = (ir_variable *)parameter_info;
                                 /* Now run the builtin function until something non-constant
   * happens or we get the result.
   */
   if (constant_expression_evaluate_expression_list(mem_ctx, origin ? origin->body : body, deref_hash, &result) &&
      result)
                     }
