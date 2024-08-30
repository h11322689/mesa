   /*
   * Copyright © 2018 Red Hat
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
   *    Rob Clark (robdclark@gmail.com)
   */
      #include "math.h"
   #include "nir/nir_builtin_builder.h"
      #include "util/u_printf.h"
   #include "vtn_private.h"
   #include "OpenCL.std.h"
      typedef nir_def *(*nir_handler)(struct vtn_builder *b,
                              static int to_llvm_address_space(SpvStorageClass mode)
   {
      switch (mode) {
   case SpvStorageClassPrivate:
   case SpvStorageClassFunction: return 0;
   case SpvStorageClassCrossWorkgroup: return 1;
   case SpvStorageClassUniform:
   case SpvStorageClassUniformConstant: return 2;
   case SpvStorageClassWorkgroup: return 3;
   case SpvStorageClassGeneric: return 4;
   default: return -1;
      }
         static void
   vtn_opencl_mangle(const char *in_name,
                     {
      char local_name[256] = "";
            for (unsigned i = 0; i < ntypes; ++i) {
      const struct glsl_type *type = src_types[i]->type;
   enum vtn_base_type base_type = src_types[i]->base_type;
   if (src_types[i]->base_type == vtn_base_type_pointer) {
      *(args_str++) = 'P';
   int address_space = to_llvm_address_space(src_types[i]->storage_class);
                  type = src_types[i]->deref->type;
               if (const_mask & (1 << i))
            unsigned num_elements = glsl_get_components(type);
   if (num_elements > 1) {
      /* Vectors are not treated as built-ins for mangling, so check for substitution.
   * In theory, we'd need to know which substitution value this is. In practice,
   * the functions we need from libclc only support 1
   */
   bool substitution = false;
   for (unsigned j = 0; j < i; ++j) {
      const struct glsl_type *other_type = src_types[j]->base_type == vtn_base_type_pointer ?
         if (type == other_type) {
      substitution = true;
                  if (substitution) {
      args_str += sprintf(args_str, "S_");
      } else
               const char *suffix = NULL;
   switch (base_type) {
   case vtn_base_type_sampler: suffix = "11ocl_sampler"; break;
   case vtn_base_type_event: suffix = "9ocl_event"; break;
   default: {
      const char *primitives[] = {
      [GLSL_TYPE_UINT] = "j",
   [GLSL_TYPE_INT] = "i",
   [GLSL_TYPE_FLOAT] = "f",
   [GLSL_TYPE_FLOAT16] = "Dh",
   [GLSL_TYPE_DOUBLE] = "d",
   [GLSL_TYPE_UINT8] = "h",
   [GLSL_TYPE_INT8] = "c",
   [GLSL_TYPE_UINT16] = "t",
   [GLSL_TYPE_INT16] = "s",
   [GLSL_TYPE_UINT64] = "m",
   [GLSL_TYPE_INT64] = "l",
   [GLSL_TYPE_BOOL] = "b",
      };
   enum glsl_base_type glsl_base_type = glsl_get_base_type(type);
   assert(glsl_base_type < ARRAY_SIZE(primitives) && primitives[glsl_base_type]);
   suffix = primitives[glsl_base_type];
      }
   }
                  }
      static nir_function *mangle_and_find(struct vtn_builder *b,
                           {
                        /* try and find in current shader first. */
            /* if not found here find in clc shader and create a decl mirroring it */
   if (!found && b->options->clc_shader && b->options->clc_shader != b->shader) {
      found = nir_shader_get_function_for_name(b->options->clc_shader, mname);
   if (found) {
      nir_function *decl = nir_function_create(b->shader, mname);
   decl->num_params = found->num_params;
   decl->params = ralloc_array(b->shader, nir_parameter, decl->num_params);
   for (unsigned i = 0; i < decl->num_params; i++) {
         }
         }
   if (!found)
         free(mname);
      }
      static bool call_mangled_function(struct vtn_builder *b,
                                    const char *name,
      {
      nir_function *found = mangle_and_find(b, name, const_mask, num_srcs, src_types);
   if (!found)
                     nir_deref_instr *ret_deref = NULL;
   uint32_t param_idx = 0;
   if (dest_type) {
      nir_variable *ret_tmp = nir_local_variable_create(b->nb.impl,
               ret_deref = nir_build_deref_var(&b->nb, ret_tmp);
               for (unsigned i = 0; i < num_srcs; i++)
                  *ret_deref_ptr = ret_deref;
      }
      static void
   handle_instr(struct vtn_builder *b, uint32_t opcode,
         {
               nir_def *srcs[5] = { NULL };
   struct vtn_type *src_types[5] = { NULL };
   vtn_assert(num_srcs <= ARRAY_SIZE(srcs));
   for (unsigned i = 0; i < num_srcs; i++) {
      struct vtn_value *val = vtn_untyped_value(b, w_src[i]);
   struct vtn_ssa_value *ssa = vtn_ssa_value(b, w_src[i]);
   srcs[i] = ssa->def;
               nir_def *result = handler(b, opcode, num_srcs, srcs, src_types, dest_type);
   if (result) {
         } else {
            }
      static nir_op
   nir_alu_op_for_opencl_opcode(struct vtn_builder *b,
         {
      switch (opcode) {
   case OpenCLstd_Fabs: return nir_op_fabs;
   case OpenCLstd_SAbs: return nir_op_iabs;
   case OpenCLstd_SAdd_sat: return nir_op_iadd_sat;
   case OpenCLstd_UAdd_sat: return nir_op_uadd_sat;
   case OpenCLstd_Ceil: return nir_op_fceil;
   case OpenCLstd_Floor: return nir_op_ffloor;
   case OpenCLstd_SHadd: return nir_op_ihadd;
   case OpenCLstd_UHadd: return nir_op_uhadd;
   case OpenCLstd_Fmax: return nir_op_fmax;
   case OpenCLstd_SMax: return nir_op_imax;
   case OpenCLstd_UMax: return nir_op_umax;
   case OpenCLstd_Fmin: return nir_op_fmin;
   case OpenCLstd_SMin: return nir_op_imin;
   case OpenCLstd_UMin: return nir_op_umin;
   case OpenCLstd_Mix: return nir_op_flrp;
   case OpenCLstd_Native_cos: return nir_op_fcos;
   case OpenCLstd_Native_divide: return nir_op_fdiv;
   case OpenCLstd_Native_exp2: return nir_op_fexp2;
   case OpenCLstd_Native_log2: return nir_op_flog2;
   case OpenCLstd_Native_powr: return nir_op_fpow;
   case OpenCLstd_Native_recip: return nir_op_frcp;
   case OpenCLstd_Native_rsqrt: return nir_op_frsq;
   case OpenCLstd_Native_sin: return nir_op_fsin;
   case OpenCLstd_Native_sqrt: return nir_op_fsqrt;
   case OpenCLstd_SMul_hi: return nir_op_imul_high;
   case OpenCLstd_UMul_hi: return nir_op_umul_high;
   case OpenCLstd_Popcount: return nir_op_bit_count;
   case OpenCLstd_SRhadd: return nir_op_irhadd;
   case OpenCLstd_URhadd: return nir_op_urhadd;
   case OpenCLstd_Rsqrt: return nir_op_frsq;
   case OpenCLstd_Sign: return nir_op_fsign;
   case OpenCLstd_Sqrt: return nir_op_fsqrt;
   case OpenCLstd_SSub_sat: return nir_op_isub_sat;
   case OpenCLstd_USub_sat: return nir_op_usub_sat;
   case OpenCLstd_Trunc: return nir_op_ftrunc;
   case OpenCLstd_Rint: return nir_op_fround_even;
   case OpenCLstd_Half_divide: return nir_op_fdiv;
   case OpenCLstd_Half_recip: return nir_op_frcp;
   /* uhm... */
   case OpenCLstd_UAbs: return nir_op_mov;
   default:
            }
      static nir_def *
   handle_alu(struct vtn_builder *b, uint32_t opcode,
               {
      nir_def *ret = nir_build_alu(&b->nb, nir_alu_op_for_opencl_opcode(b, (enum OpenCLstd_Entrypoints)opcode),
         if (opcode == OpenCLstd_Popcount)
            }
      #define REMAP(op, str) [OpenCLstd_##op] = { str }
   static const struct {
         } remap_table[] = {
      REMAP(Distance, "distance"),
   REMAP(Fast_distance, "fast_distance"),
   REMAP(Fast_length, "fast_length"),
   REMAP(Fast_normalize, "fast_normalize"),
   REMAP(Half_rsqrt, "half_rsqrt"),
   REMAP(Half_sqrt, "half_sqrt"),
   REMAP(Length, "length"),
   REMAP(Normalize, "normalize"),
   REMAP(Degrees, "degrees"),
   REMAP(Radians, "radians"),
   REMAP(Rotate, "rotate"),
   REMAP(Smoothstep, "smoothstep"),
            REMAP(Pow, "pow"),
   REMAP(Pown, "pown"),
   REMAP(Powr, "powr"),
   REMAP(Rootn, "rootn"),
            REMAP(Acos, "acos"),
   REMAP(Acosh, "acosh"),
   REMAP(Acospi, "acospi"),
   REMAP(Asin, "asin"),
   REMAP(Asinh, "asinh"),
   REMAP(Asinpi, "asinpi"),
   REMAP(Atan, "atan"),
   REMAP(Atan2, "atan2"),
   REMAP(Atanh, "atanh"),
   REMAP(Atanpi, "atanpi"),
   REMAP(Atan2pi, "atan2pi"),
   REMAP(Cos, "cos"),
   REMAP(Cosh, "cosh"),
   REMAP(Cospi, "cospi"),
   REMAP(Sin, "sin"),
   REMAP(Sinh, "sinh"),
   REMAP(Sinpi, "sinpi"),
   REMAP(Tan, "tan"),
   REMAP(Tanh, "tanh"),
   REMAP(Tanpi, "tanpi"),
   REMAP(Sincos, "sincos"),
   REMAP(Fract, "fract"),
   REMAP(Frexp, "frexp"),
   REMAP(Fma, "fma"),
            REMAP(Half_cos, "cos"),
   REMAP(Half_exp, "exp"),
   REMAP(Half_exp2, "exp2"),
   REMAP(Half_exp10, "exp10"),
   REMAP(Half_log, "log"),
   REMAP(Half_log2, "log2"),
   REMAP(Half_log10, "log10"),
   REMAP(Half_powr, "powr"),
   REMAP(Half_sin, "sin"),
            REMAP(Remainder, "remainder"),
   REMAP(Remquo, "remquo"),
   REMAP(Hypot, "hypot"),
   REMAP(Exp, "exp"),
   REMAP(Exp2, "exp2"),
   REMAP(Exp10, "exp10"),
   REMAP(Expm1, "expm1"),
            REMAP(Ilogb, "ilogb"),
   REMAP(Log, "log"),
   REMAP(Log2, "log2"),
   REMAP(Log10, "log10"),
   REMAP(Log1p, "log1p"),
            REMAP(Cbrt, "cbrt"),
   REMAP(Erfc, "erfc"),
            REMAP(Lgamma, "lgamma"),
   REMAP(Lgamma_r, "lgamma_r"),
            REMAP(UMad_sat, "mad_sat"),
            REMAP(Shuffle, "shuffle"),
      };
   #undef REMAP
      static const char *remap_clc_opcode(enum OpenCLstd_Entrypoints opcode)
   {
      if (opcode >= (sizeof(remap_table) / sizeof(const char *)))
            }
      static struct vtn_type *
   get_vtn_type_for_glsl_type(struct vtn_builder *b, const struct glsl_type *type)
   {
      struct vtn_type *ret = rzalloc(b, struct vtn_type);
   assert(glsl_type_is_vector_or_scalar(type));
   ret->type = type;
   ret->length = glsl_get_vector_elements(type);
   ret->base_type = glsl_type_is_vector(type) ? vtn_base_type_vector : vtn_base_type_scalar;
      }
      static struct vtn_type *
   get_pointer_type(struct vtn_builder *b, struct vtn_type *t, SpvStorageClass storage_class)
   {
      struct vtn_type *ret = rzalloc(b, struct vtn_type);
   ret->type = nir_address_format_to_glsl_type(
               ret->base_type = vtn_base_type_pointer;
   ret->storage_class = storage_class;
   ret->deref = t;
      }
      static struct vtn_type *
   get_signed_type(struct vtn_builder *b, struct vtn_type *t)
   {
      if (t->base_type == vtn_base_type_pointer) {
         }
   return get_vtn_type_for_glsl_type(
      b, glsl_vector_type(glsl_signed_base_type_of(glsl_get_base_type(t->type)),
   }
      static nir_def *
   handle_clc_fn(struct vtn_builder *b, enum OpenCLstd_Entrypoints opcode,
               int num_srcs,
   nir_def **srcs,
   {
      const char *name = remap_clc_opcode(opcode);
   if (!name)
            /* Some functions which take params end up with uint (or pointer-to-uint) being passed,
   * which doesn't mangle correctly when the function expects int or pointer-to-int.
   * See https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_a_id_unsignedsigned_a_unsigned_versus_signed_integers
   */
   int signed_param = -1;
   switch (opcode) {
   case OpenCLstd_Frexp:
   case OpenCLstd_Lgamma_r:
   case OpenCLstd_Pown:
   case OpenCLstd_Rootn:
   case OpenCLstd_Ldexp:
      signed_param = 1;
      case OpenCLstd_Remquo:
      signed_param = 2;
      case OpenCLstd_SMad_sat: {
      /* All parameters need to be converted to signed */
   src_types[0] = src_types[1] = src_types[2] = get_signed_type(b, src_types[0]);
      }
   default: break;
            if (signed_param >= 0) {
                           if (!call_mangled_function(b, name, 0, num_srcs, src_types,
                     }
      static nir_def *
   handle_special(struct vtn_builder *b, uint32_t opcode,
               {
      nir_builder *nb = &b->nb;
            switch (cl_opcode) {
   case OpenCLstd_SAbs_diff:
   /* these works easier in direct NIR */
         case OpenCLstd_UAbs_diff:
         case OpenCLstd_Bitselect:
         case OpenCLstd_SMad_hi:
         case OpenCLstd_UMad_hi:
         case OpenCLstd_SMul24:
         case OpenCLstd_UMul24:
         case OpenCLstd_SMad24:
         case OpenCLstd_UMad24:
         case OpenCLstd_FClamp:
         case OpenCLstd_SClamp:
         case OpenCLstd_UClamp:
         case OpenCLstd_Copysign:
         case OpenCLstd_Cross:
      if (dest_type->length == 4)
            case OpenCLstd_Fdim:
         case OpenCLstd_Fmod:
      if (nb->shader->options->lower_fmod)
            case OpenCLstd_Mad:
         case OpenCLstd_Maxmag:
         case OpenCLstd_Minmag:
         case OpenCLstd_Nan:
         case OpenCLstd_Nextafter:
         case OpenCLstd_Normalize:
         case OpenCLstd_Clz:
         case OpenCLstd_Ctz:
         case OpenCLstd_Select:
         case OpenCLstd_S_Upsample:
   case OpenCLstd_U_Upsample:
      /* SPIR-V and CL have different defs for upsample, just implement in nir */
      case OpenCLstd_Native_exp:
         case OpenCLstd_Native_exp10:
         case OpenCLstd_Native_log:
         case OpenCLstd_Native_log10:
         case OpenCLstd_Native_tan:
         case OpenCLstd_Ldexp:
      if (nb->shader->options->lower_ldexp)
            case OpenCLstd_Fma:
      /* FIXME: the software implementation only supports fp32 for now. */
   if (nb->shader->options->lower_ffma32 && srcs[0]->bit_size == 32)
            case OpenCLstd_Rotate:
         default:
                  nir_def *ret = handle_clc_fn(b, opcode, num_srcs, srcs, src_types, dest_type);
   if (!ret)
               }
      static nir_def *
   handle_core(struct vtn_builder *b, uint32_t opcode,
               {
               switch ((SpvOp)opcode) {
   case SpvOpGroupAsyncCopy: {
      /* Libclc doesn't include 3-component overloads of the async copy functions.
   * However, the CLC spec says:
   * async_work_group_copy and async_work_group_strided_copy for 3-component vector types
   * behave as async_work_group_copy and async_work_group_strided_copy respectively for 4-component
   * vector types
   */
   for (unsigned i = 0; i < num_srcs; ++i) {
      if (src_types[i]->base_type == vtn_base_type_pointer &&
      src_types[i]->deref->base_type == vtn_base_type_vector &&
   src_types[i]->deref->length == 3) {
   src_types[i] =
      get_pointer_type(b,
            }
   if (!call_mangled_function(b, "async_work_group_strided_copy", (1 << 1), num_srcs, src_types, dest_type, srcs, &ret_deref))
            }
   case SpvOpGroupWaitEvents: {
      /* libclc and clang don't agree on the mangling of this function.
   * The libclc we have uses a __local pointer but clang gives us generic
   * pointers.  Fortunately, the whole function is just a barrier.
   */
   nir_barrier(&b->nb, .execution_scope = SCOPE_WORKGROUP,
                     .memory_scope = SCOPE_WORKGROUP,
   .memory_semantics = NIR_MEMORY_ACQUIRE |
      }
   default:
                     }
         static void
   _handle_v_load_store(struct vtn_builder *b, enum OpenCLstd_Entrypoints opcode,
               {
      struct vtn_type *type;
   if (load)
         else
                  enum glsl_base_type base_type = glsl_get_base_type(type->type);
            nir_def *offset = vtn_get_nir_ssa(b, w[5 + a]);
            struct vtn_ssa_value *comps[NIR_MAX_VEC_COMPONENTS];
            nir_def *moffset = nir_imul_imm(&b->nb, offset,
                  unsigned alignment = vec_aligned ? glsl_get_cl_alignment(type->type) :
         enum glsl_base_type ptr_base_type =
         if (base_type != ptr_base_type) {
      vtn_fail_if(ptr_base_type != GLSL_TYPE_FLOAT16 ||
               (base_type != GLSL_TYPE_FLOAT &&
   base_type != GLSL_TYPE_DOUBLE),
            /* Above-computed alignment was for floats/doubles, not halves */
                        for (int i = 0; i < components; i++) {
      nir_def *coffset = nir_iadd_imm(&b->nb, moffset, i);
            if (load) {
      comps[i] = vtn_local_load(b, arr_deref, p->type->access);
   ncomps[i] = comps[i]->def;
   if (base_type != ptr_base_type) {
      assert(ptr_base_type == GLSL_TYPE_FLOAT16 &&
         (base_type == GLSL_TYPE_FLOAT ||
   ncomps[i] = nir_f2fN(&b->nb, ncomps[i],
         } else {
      struct vtn_ssa_value *ssa = vtn_create_ssa_value(b, glsl_scalar_type(base_type));
   struct vtn_ssa_value *val = vtn_ssa_value(b, w[5]);
   ssa->def = nir_channel(&b->nb, val->def, i);
   if (base_type != ptr_base_type) {
      assert(ptr_base_type == GLSL_TYPE_FLOAT16 &&
         (base_type == GLSL_TYPE_FLOAT ||
   if (rounding == nir_rounding_mode_undef) {
         } else {
      ssa->def = nir_convert_alu_types(&b->nb, 16, ssa->def,
                     }
         }
   if (load) {
            }
      static void
   vtn_handle_opencl_vload(struct vtn_builder *b, enum OpenCLstd_Entrypoints opcode,
         {
      _handle_v_load_store(b, opcode, w, count, true,
            }
      static void
   vtn_handle_opencl_vstore(struct vtn_builder *b, enum OpenCLstd_Entrypoints opcode,
         {
      _handle_v_load_store(b, opcode, w, count, false,
            }
      static void
   vtn_handle_opencl_vstore_half_r(struct vtn_builder *b, enum OpenCLstd_Entrypoints opcode,
         {
      _handle_v_load_store(b, opcode, w, count, false,
            }
      static unsigned
   vtn_add_printf_string(struct vtn_builder *b, uint32_t id, u_printf_info *info)
   {
               while (deref && deref->deref_type != nir_deref_type_var)
            vtn_fail_if(deref == NULL || !nir_deref_mode_is(deref, nir_var_mem_constant),
         vtn_fail_if(deref->var->constant_initializer == NULL,
         vtn_fail_if(!glsl_type_is_array(deref->var->type),
         const struct glsl_type *char_type = glsl_get_array_element(deref->var->type);
   vtn_fail_if(char_type != glsl_uint8_t_type() &&
                  nir_constant *c = deref->var->constant_initializer;
            unsigned idx = info->string_size;
   info->strings = reralloc_size(b->shader, info->strings,
                  char *str = &info->strings[idx];
   bool found_null = false;
   for (unsigned i = 0; i < c->num_elements; i++) {
      memcpy((char *)str + i, c->elements[i]->values, 1);
   if (str[i] == '\0')
      }
   vtn_fail_if(!found_null, "Printf string must be null terminated");
      }
      /* printf is special because there are no limits on args */
   static void
   handle_printf(struct vtn_builder *b, uint32_t opcode,
         {
      if (!b->options->caps.printf) {
      vtn_push_nir_ssa(b, w_dest[1], nir_imm_int(&b->nb, -1));
                        /*
   * info_idx is 1-based to match clover/llvm
   * the backend indexes the info table at info_idx - 1.
   */
   b->shader->printf_info_count++;
            b->shader->printf_info = reralloc(b->shader, b->shader->printf_info,
                  info->strings = NULL;
                     info->num_args = num_srcs - 1;
            /* Step 2, build an ad-hoc struct type out of the args */
   unsigned field_offset = 0;
   struct glsl_struct_field *fields =
         for (unsigned i = 1; i < num_srcs; ++i) {
      struct vtn_value *val = vtn_untyped_value(b, w_src[i]);
   struct vtn_type *src_type = val->type;
   fields[i - 1].type = src_type->type;
   fields[i - 1].name = ralloc_asprintf(b->shader, "arg_%u", i);
   field_offset = align(field_offset, 4);
   fields[i - 1].offset = field_offset;
   info->arg_sizes[i - 1] = glsl_get_cl_size(src_type->type);
      }
   const struct glsl_type *struct_type =
            /* Step 3, create a variable of that type and populate its fields */
   nir_variable *var = nir_local_variable_create(b->nb.impl, struct_type, NULL);
   nir_deref_instr *deref_var = nir_build_deref_var(&b->nb, var);
   size_t fmt_pos = 0;
   for (unsigned i = 1; i < num_srcs; ++i) {
      nir_deref_instr *field_deref =
         nir_def *field_src = vtn_ssa_value(b, w_src[i])->def;
   /* extract strings */
   fmt_pos = util_printf_next_spec_pos(info->strings, fmt_pos);
   if (fmt_pos != -1 && info->strings[fmt_pos] == 's') {
      unsigned idx = vtn_add_printf_string(b, w_src[i], info);
   nir_store_deref(&b->nb, field_deref,
            } else
               /* Lastly, the actual intrinsic */
   nir_def *fmt_idx = nir_imm_int(&b->nb, info_idx);
   nir_def *ret = nir_printf(&b->nb, fmt_idx, &deref_var->def);
      }
      static nir_def *
   handle_round(struct vtn_builder *b, uint32_t opcode,
               {
      nir_def *src = srcs[0];
   nir_builder *nb = &b->nb;
   nir_def *half = nir_imm_floatN_t(nb, 0.5, src->bit_size);
   nir_def *truncated = nir_ftrunc(nb, src);
            return nir_bcsel(nb, nir_fge(nb, nir_fabs(nb, remainder), half),
      }
      static nir_def *
   handle_shuffle(struct vtn_builder *b, uint32_t opcode,
               {
      struct nir_def *input = srcs[0];
            unsigned out_elems = dest_type->length;
   nir_def *outres[NIR_MAX_VEC_COMPONENTS];
   unsigned in_elems = input->num_components;
   if (mask->bit_size != 32)
         mask = nir_iand(&b->nb, mask, nir_imm_intN_t(&b->nb, in_elems - 1, mask->bit_size));
   for (unsigned i = 0; i < out_elems; i++)
               }
      static nir_def *
   handle_shuffle2(struct vtn_builder *b, uint32_t opcode,
               {
      struct nir_def *input0 = srcs[0];
   struct nir_def *input1 = srcs[1];
            unsigned out_elems = dest_type->length;
   nir_def *outres[NIR_MAX_VEC_COMPONENTS];
   unsigned in_elems = input0->num_components;
   unsigned total_mask = 2 * in_elems - 1;
   unsigned half_mask = in_elems - 1;
   if (mask->bit_size != 32)
         mask = nir_iand(&b->nb, mask, nir_imm_intN_t(&b->nb, total_mask, mask->bit_size));
   for (unsigned i = 0; i < out_elems; i++) {
      nir_def *this_mask = nir_channel(&b->nb, mask, i);
   nir_def *vmask = nir_iand(&b->nb, this_mask, nir_imm_intN_t(&b->nb, half_mask, mask->bit_size));
   nir_def *val0 = nir_vector_extract(&b->nb, input0, vmask);
   nir_def *val1 = nir_vector_extract(&b->nb, input1, vmask);
   nir_def *sel = nir_ilt_imm(&b->nb, this_mask, in_elems);
      }
      }
      bool
   vtn_handle_opencl_instruction(struct vtn_builder *b, SpvOp ext_opcode,
         {
               switch (cl_opcode) {
   case OpenCLstd_Fabs:
   case OpenCLstd_SAbs:
   case OpenCLstd_UAbs:
   case OpenCLstd_SAdd_sat:
   case OpenCLstd_UAdd_sat:
   case OpenCLstd_Ceil:
   case OpenCLstd_Floor:
   case OpenCLstd_Fmax:
   case OpenCLstd_SHadd:
   case OpenCLstd_UHadd:
   case OpenCLstd_SMax:
   case OpenCLstd_UMax:
   case OpenCLstd_Fmin:
   case OpenCLstd_SMin:
   case OpenCLstd_UMin:
   case OpenCLstd_Mix:
   case OpenCLstd_Native_cos:
   case OpenCLstd_Native_divide:
   case OpenCLstd_Native_exp2:
   case OpenCLstd_Native_log2:
   case OpenCLstd_Native_powr:
   case OpenCLstd_Native_recip:
   case OpenCLstd_Native_rsqrt:
   case OpenCLstd_Native_sin:
   case OpenCLstd_Native_sqrt:
   case OpenCLstd_SMul_hi:
   case OpenCLstd_UMul_hi:
   case OpenCLstd_Popcount:
   case OpenCLstd_SRhadd:
   case OpenCLstd_URhadd:
   case OpenCLstd_Rsqrt:
   case OpenCLstd_Sign:
   case OpenCLstd_Sqrt:
   case OpenCLstd_SSub_sat:
   case OpenCLstd_USub_sat:
   case OpenCLstd_Trunc:
   case OpenCLstd_Rint:
   case OpenCLstd_Half_divide:
   case OpenCLstd_Half_recip:
      handle_instr(b, ext_opcode, w + 5, count - 5, w + 1, handle_alu);
      case OpenCLstd_SAbs_diff:
   case OpenCLstd_UAbs_diff:
   case OpenCLstd_SMad_hi:
   case OpenCLstd_UMad_hi:
   case OpenCLstd_SMad24:
   case OpenCLstd_UMad24:
   case OpenCLstd_SMul24:
   case OpenCLstd_UMul24:
   case OpenCLstd_Bitselect:
   case OpenCLstd_FClamp:
   case OpenCLstd_SClamp:
   case OpenCLstd_UClamp:
   case OpenCLstd_Copysign:
   case OpenCLstd_Cross:
   case OpenCLstd_Degrees:
   case OpenCLstd_Fdim:
   case OpenCLstd_Fma:
   case OpenCLstd_Distance:
   case OpenCLstd_Fast_distance:
   case OpenCLstd_Fast_length:
   case OpenCLstd_Fast_normalize:
   case OpenCLstd_Half_rsqrt:
   case OpenCLstd_Half_sqrt:
   case OpenCLstd_Length:
   case OpenCLstd_Mad:
   case OpenCLstd_Maxmag:
   case OpenCLstd_Minmag:
   case OpenCLstd_Nan:
   case OpenCLstd_Nextafter:
   case OpenCLstd_Normalize:
   case OpenCLstd_Radians:
   case OpenCLstd_Rotate:
   case OpenCLstd_Select:
   case OpenCLstd_Step:
   case OpenCLstd_Smoothstep:
   case OpenCLstd_S_Upsample:
   case OpenCLstd_U_Upsample:
   case OpenCLstd_Clz:
   case OpenCLstd_Ctz:
   case OpenCLstd_Native_exp:
   case OpenCLstd_Native_exp10:
   case OpenCLstd_Native_log:
   case OpenCLstd_Native_log10:
   case OpenCLstd_Acos:
   case OpenCLstd_Acosh:
   case OpenCLstd_Acospi:
   case OpenCLstd_Asin:
   case OpenCLstd_Asinh:
   case OpenCLstd_Asinpi:
   case OpenCLstd_Atan:
   case OpenCLstd_Atan2:
   case OpenCLstd_Atanh:
   case OpenCLstd_Atanpi:
   case OpenCLstd_Atan2pi:
   case OpenCLstd_Fract:
   case OpenCLstd_Frexp:
   case OpenCLstd_Exp:
   case OpenCLstd_Exp2:
   case OpenCLstd_Expm1:
   case OpenCLstd_Exp10:
   case OpenCLstd_Fmod:
   case OpenCLstd_Ilogb:
   case OpenCLstd_Log:
   case OpenCLstd_Log2:
   case OpenCLstd_Log10:
   case OpenCLstd_Log1p:
   case OpenCLstd_Logb:
   case OpenCLstd_Ldexp:
   case OpenCLstd_Cos:
   case OpenCLstd_Cosh:
   case OpenCLstd_Cospi:
   case OpenCLstd_Sin:
   case OpenCLstd_Sinh:
   case OpenCLstd_Sinpi:
   case OpenCLstd_Tan:
   case OpenCLstd_Tanh:
   case OpenCLstd_Tanpi:
   case OpenCLstd_Cbrt:
   case OpenCLstd_Erfc:
   case OpenCLstd_Erf:
   case OpenCLstd_Lgamma:
   case OpenCLstd_Lgamma_r:
   case OpenCLstd_Tgamma:
   case OpenCLstd_Pow:
   case OpenCLstd_Powr:
   case OpenCLstd_Pown:
   case OpenCLstd_Rootn:
   case OpenCLstd_Remainder:
   case OpenCLstd_Remquo:
   case OpenCLstd_Hypot:
   case OpenCLstd_Sincos:
   case OpenCLstd_Modf:
   case OpenCLstd_UMad_sat:
   case OpenCLstd_SMad_sat:
   case OpenCLstd_Native_tan:
   case OpenCLstd_Half_cos:
   case OpenCLstd_Half_exp:
   case OpenCLstd_Half_exp2:
   case OpenCLstd_Half_exp10:
   case OpenCLstd_Half_log:
   case OpenCLstd_Half_log2:
   case OpenCLstd_Half_log10:
   case OpenCLstd_Half_powr:
   case OpenCLstd_Half_sin:
   case OpenCLstd_Half_tan:
      handle_instr(b, ext_opcode, w + 5, count - 5, w + 1, handle_special);
      case OpenCLstd_Vloadn:
   case OpenCLstd_Vload_half:
   case OpenCLstd_Vload_halfn:
   case OpenCLstd_Vloada_halfn:
      vtn_handle_opencl_vload(b, cl_opcode, w, count);
      case OpenCLstd_Vstoren:
   case OpenCLstd_Vstore_half:
   case OpenCLstd_Vstore_halfn:
   case OpenCLstd_Vstorea_halfn:
      vtn_handle_opencl_vstore(b, cl_opcode, w, count);
      case OpenCLstd_Vstore_half_r:
   case OpenCLstd_Vstore_halfn_r:
   case OpenCLstd_Vstorea_halfn_r:
      vtn_handle_opencl_vstore_half_r(b, cl_opcode, w, count);
      case OpenCLstd_Shuffle:
      handle_instr(b, ext_opcode, w + 5, count - 5, w + 1, handle_shuffle);
      case OpenCLstd_Shuffle2:
      handle_instr(b, ext_opcode, w + 5, count - 5, w + 1, handle_shuffle2);
      case OpenCLstd_Round:
      handle_instr(b, ext_opcode, w + 5, count - 5, w + 1, handle_round);
      case OpenCLstd_Printf:
      handle_printf(b, ext_opcode, w + 5, count - 5, w + 1);
      case OpenCLstd_Prefetch:
      /* TODO maybe add a nir instruction for this? */
      default:
      vtn_fail("unhandled opencl opc: %u\n", ext_opcode);
         }
      bool
   vtn_handle_opencl_core_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpGroupAsyncCopy:
      handle_instr(b, opcode, w + 4, count - 4, w + 1, handle_core);
      case SpvOpGroupWaitEvents:
      handle_instr(b, opcode, w + 2, count - 2, NULL, handle_core);
      default:
         }
      }
