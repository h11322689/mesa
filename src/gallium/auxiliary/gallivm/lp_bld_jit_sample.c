   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "gallivm/lp_bld_sample.h"
   #include "gallivm/lp_bld_limits.h"
   #include "gallivm/lp_bld_tgsi.h"
   #include "gallivm/lp_bld_type.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_sample.h"
   #include "gallivm/lp_bld_jit_types.h"
   #include "gallivm/lp_bld_jit_sample.h"
   #include "gallivm/lp_bld_flow.h"
      struct lp_bld_sampler_dynamic_state
   {
                  };
      struct lp_bld_llvm_sampler_soa
   {
               struct lp_bld_sampler_dynamic_state dynamic_state;
      };
         struct lp_bld_image_dynamic_state
   {
                  };
      struct lp_bld_llvm_image_soa
   {
               struct lp_bld_image_dynamic_state dynamic_state;
      };
      static LLVMValueRef
   load_texture_functions_ptr(struct gallivm_state *gallivm, LLVMValueRef descriptor,
         {
               LLVMValueRef texture_base_offset = lp_build_const_int64(gallivm, offset1);
            LLVMTypeRef texture_base_type = LLVMInt64TypeInContext(gallivm->context);
            texture_base_ptr = LLVMBuildIntToPtr(builder, texture_base_ptr, texture_base_ptr_type, "");
   /* struct lp_texture_functions * */
            LLVMValueRef functions_offset = lp_build_const_int64(gallivm, offset2);
      }
      static LLVMValueRef
   widen_to_simd_width(struct gallivm_state *gallivm, LLVMValueRef value)
   {
      LLVMBuilderRef builder = gallivm->builder;
            if (LLVMGetTypeKind(type) == LLVMVectorTypeKind) {
      LLVMTypeRef element_type = LLVMGetElementType(type);
   uint32_t element_count = LLVMGetVectorSize(type);
   LLVMValueRef elements[8] = { 0 };
   for (uint32_t i = 0; i < lp_native_vector_width / 32; i++) {
      if (i < element_count)
         else
               LLVMTypeRef result_type = LLVMVectorType(element_type, lp_native_vector_width / 32);
   LLVMValueRef result = LLVMGetUndef(result_type);
   for (unsigned i = 0; i < lp_native_vector_width / 32; i++)
                           }
      static LLVMValueRef
   truncate_to_type_width(struct gallivm_state *gallivm, LLVMValueRef value, struct lp_type target_type)
   {
      LLVMBuilderRef builder = gallivm->builder;
            if (LLVMGetTypeKind(type) == LLVMVectorTypeKind) {
               LLVMValueRef elements[8];
   for (uint32_t i = 0; i < target_type.length; i++)
            LLVMTypeRef result_type = LLVMVectorType(element_type, target_type.length);
   LLVMValueRef result = LLVMGetUndef(result_type);
   for (unsigned i = 0; i < target_type.length; i++)
                           }
      /**
   * Fetch filtered values from texture.
   * The 'texel' parameter returns four vectors corresponding to R, G, B, A.
   */
   static void
   lp_bld_llvm_sampler_soa_emit_fetch_texel(const struct lp_build_sampler_soa *base,
               {
      struct lp_bld_llvm_sampler_soa *sampler = (struct lp_bld_llvm_sampler_soa *)base;
            if (params->texture_resource) {
               LLVMValueRef out_data[4];
   for (uint32_t i = 0; i < 4; i++) {
      out_data[i] = lp_build_alloca(gallivm, out_data_type, "");
               struct lp_type uint_type = lp_uint_type(params->type);
                     LLVMTypeRef bitmask_type = LLVMIntTypeInContext(gallivm->context, uint_type.length);
                     struct lp_build_if_state if_state;
                              enum lp_sampler_op_type op_type = (params->sample_key & LP_SAMPLER_OP_TYPE_MASK) >> LP_SAMPLER_OP_TYPE_SHIFT;
   uint32_t functions_offset = op_type == LP_SAMPLER_OP_FETCH ? offsetof(struct lp_texture_functions, fetch_functions)
            LLVMValueRef texture_base_ptr = load_texture_functions_ptr(
            LLVMTypeRef texture_function_type = lp_build_sample_function_type(gallivm, params->sample_key);
   LLVMTypeRef texture_function_ptr_type = LLVMPointerType(texture_function_type, 0);
   LLVMTypeRef texture_functions_type = LLVMPointerType(texture_function_ptr_type, 0);
   LLVMTypeRef texture_base_type = LLVMPointerType(texture_functions_type, 0);
            texture_base_ptr = LLVMBuildIntToPtr(builder, texture_base_ptr, texture_base_ptr_type, "");
            LLVMValueRef texture_functions;
   LLVMValueRef sampler_desc_ptr;
   if (op_type == LP_SAMPLER_OP_FETCH) {
      texture_functions = texture_base;
      } else {
                                                            LLVMValueRef texture_functions_ptr = LLVMBuildGEP2(builder, texture_functions_type, texture_base, &sampler_index, 1, "");
               LLVMValueRef sample_key = lp_build_const_int32(gallivm, params->sample_key);
   LLVMValueRef texture_function_ptr = LLVMBuildGEP2(builder, texture_function_ptr_type, texture_functions, &sample_key, 1, "");
            LLVMValueRef args[LP_MAX_TEX_FUNC_ARGS];
            args[num_args++] = texture_descriptor;
                     LLVMTypeRef coord_type;
   if (op_type == LP_SAMPLER_OP_FETCH)
         else
            for (uint32_t i = 0; i < 4; i++) {
      if (LLVMIsUndef(params->coords[i]))
         else
               if (params->sample_key & LP_SAMPLER_SHADOW)
            if (params->sample_key & LP_SAMPLER_FETCH_MS)
            if (params->sample_key & LP_SAMPLER_OFFSETS) {
      for (uint32_t i = 0; i < 3; i++) {
      if (params->offsets[i])
         else
                  enum lp_sampler_lod_control lod_control = (params->sample_key & LP_SAMPLER_LOD_CONTROL_MASK) >> LP_SAMPLER_LOD_CONTROL_SHIFT;
   if (lod_control == LP_SAMPLER_LOD_BIAS || lod_control == LP_SAMPLER_LOD_EXPLICIT)
            if (params->type.length != lp_native_vector_width / 32)
                           for (unsigned i = 0; i < 4; i++) {
                                                   for (unsigned i = 0; i < 4; i++)
                        const unsigned texture_index = params->texture_index;
            assert(sampler_index < PIPE_MAX_SAMPLERS);
      #if 0
      if (LP_PERF & PERF_NO_TEX) {
      lp_build_sample_nop(gallivm, params->type, params->coords, params->texel);
         #endif
         if (params->texture_index_offset) {
      LLVMValueRef unit =
                  struct lp_build_sample_array_switch switch_info;
   memset(&switch_info, 0, sizeof(switch_info));
   lp_build_sample_array_init_soa(&switch_info, gallivm, params, unit,
         // build the switch cases
   for (unsigned i = 0; i < sampler->nr_samplers; i++) {
      lp_build_sample_array_case_soa(&switch_info, i,
                  }
      } else {
      lp_build_sample_soa(&sampler->dynamic_state.static_state[texture_index].texture_state,
                     }
         /**
   * Fetch the texture size.
   */
   static void
   lp_bld_llvm_sampler_soa_emit_size_query(const struct lp_build_sampler_soa *base,
               {
               if (params->resource) {
               LLVMValueRef out_data[4];
   for (uint32_t i = 0; i < 4; i++) {
      out_data[i] = lp_build_alloca(gallivm, out_data_type, "");
               struct lp_type uint_type = lp_uint_type(params->int_type);
                     LLVMTypeRef bitmask_type = LLVMIntTypeInContext(gallivm->context, uint_type.length);
                     struct lp_build_if_state if_state;
                              uint32_t functions_offset = params->samples_only ? offsetof(struct lp_texture_functions, samples_function)
            LLVMValueRef texture_base_ptr = load_texture_functions_ptr(
            LLVMTypeRef texture_function_type = lp_build_size_function_type(gallivm, params);
   LLVMTypeRef texture_function_ptr_type = LLVMPointerType(texture_function_type, 0);
            texture_base_ptr = LLVMBuildIntToPtr(builder, texture_base_ptr, texture_function_ptr_ptr_type, "");
            LLVMValueRef args[LP_MAX_TEX_FUNC_ARGS];
                     if (!params->samples_only)
            if (params->int_type.length != lp_native_vector_width / 32)
                           for (unsigned i = 0; i < 4; i++) {
                                                   for (unsigned i = 0; i < 4; i++)
                                          lp_build_size_query_soa(gallivm,
                  }
         struct lp_build_sampler_soa *
   lp_bld_llvm_sampler_soa_create(const struct lp_sampler_static_state *static_state,
         {
               struct lp_bld_llvm_sampler_soa *sampler = CALLOC_STRUCT(lp_bld_llvm_sampler_soa);
   if (!sampler)
            sampler->base.emit_tex_sample = lp_bld_llvm_sampler_soa_emit_fetch_texel;
                              sampler->nr_samplers = nr_samplers;
      }
         static void
   lp_bld_llvm_image_soa_emit_op(const struct lp_build_image_soa *base,
               {
               if (params->resource) {
      const struct util_format_description *desc = util_format_description(params->format);
            LLVMValueRef out_data[4];
   for (uint32_t i = 0; i < 4; i++) {
      out_data[i] = lp_build_alloca(gallivm, out_data_type, "");
               struct lp_type uint_type = lp_uint_type(params->type);
                     LLVMTypeRef bitmask_type = LLVMIntTypeInContext(gallivm->context, uint_type.length);
                     LLVMValueRef binding_index = LLVMBuildExtractValue(builder, params->resource, 1, "");
            struct lp_build_if_state if_state;
                              LLVMValueRef image_base_ptr = load_texture_functions_ptr(
                  LLVMTypeRef image_function_type = lp_build_image_function_type(gallivm, params, params->ms_index);
   LLVMTypeRef image_function_ptr_type = LLVMPointerType(image_function_type, 0);
   LLVMTypeRef image_functions_type = LLVMPointerType(image_function_ptr_type, 0);
            image_base_ptr = LLVMBuildIntToPtr(builder, image_base_ptr, image_base_type, "");
            uint32_t op = params->img_op;
   if (op == LP_IMG_ATOMIC_CAS)
         else if (op == LP_IMG_ATOMIC)
            if (params->ms_index)
                     LLVMValueRef image_function_ptr = LLVMBuildGEP2(builder, image_function_ptr_type, image_functions, &function_index, 1, "");
            LLVMValueRef args[LP_MAX_TEX_FUNC_ARGS] = { 0 };
                     if (params->img_op != LP_IMG_LOAD)
            for (uint32_t i = 0; i < 3; i++)
            if (params->ms_index)
            if (params->img_op != LP_IMG_LOAD)
                  if (params->img_op == LP_IMG_ATOMIC_CAS)
                           LLVMTypeRef param_types[LP_MAX_TEX_FUNC_ARGS];
   LLVMGetParamTypes(image_function_type, param_types);
   for (uint32_t i = 0; i < num_args; i++)
                  if (params->type.length != lp_native_vector_width / 32)
                           if (params->img_op != LP_IMG_STORE) {
      for (unsigned i = 0; i < 4; i++) {
      LLVMValueRef channel = LLVMBuildExtractValue(gallivm->builder, result, i, "");
                                          if (params->img_op != LP_IMG_STORE) {
      for (unsigned i = 0; i < 4; i++) {
                                 struct lp_bld_llvm_image_soa *image = (struct lp_bld_llvm_image_soa *)base;
   const unsigned image_index = params->image_index;
            if (params->image_index_offset) {
      struct lp_build_img_op_array_switch switch_info;
   memset(&switch_info, 0, sizeof(switch_info));
   LLVMValueRef unit = LLVMBuildAdd(gallivm->builder,
                        lp_build_image_op_switch_soa(&switch_info, gallivm, params,
            for (unsigned i = 0; i < image->nr_images; i++) {
      lp_build_image_op_array_case(&switch_info, i,
            }
      } else {
      lp_build_img_op_soa(&image->dynamic_state.static_state[image_index].image_state,
               }
         /**
   * Fetch the texture size.
   */
   static void
   lp_bld_llvm_image_soa_emit_size_query(const struct lp_build_image_soa *base,
               {
               if (params->resource) {
               LLVMValueRef consts = lp_jit_resources_constants(gallivm, params->resources_type, params->resources_ptr);
            enum pipe_format format = params->format;
   if (format == PIPE_FORMAT_NONE)
            struct lp_static_texture_state state = {
      .format = format,
   .res_format = format,
      };
                                             lp_build_size_query_soa(gallivm,
                  }
         struct lp_build_image_soa *
   lp_bld_llvm_image_soa_create(const struct lp_image_static_state *static_state,
         {
      struct lp_bld_llvm_image_soa *image = CALLOC_STRUCT(lp_bld_llvm_image_soa);
   if (!image)
            image->base.emit_op = lp_bld_llvm_image_soa_emit_op;
            lp_build_jit_fill_image_dynamic_state(&image->dynamic_state.base);
            image->nr_images = nr_images;
      }
      struct lp_sampler_dynamic_state *
   lp_build_sampler_soa_dynamic_state(struct lp_build_sampler_soa *_sampler)
   {
      struct lp_bld_llvm_sampler_soa *sampler = (struct lp_bld_llvm_sampler_soa *)_sampler;
      }
      struct lp_sampler_dynamic_state *
   lp_build_image_soa_dynamic_state(struct lp_build_image_soa *_image)
   {
      struct lp_bld_llvm_image_soa *image = (struct lp_bld_llvm_image_soa *)_image;
      }
