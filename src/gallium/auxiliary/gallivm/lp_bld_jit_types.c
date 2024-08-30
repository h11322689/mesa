   /*
   * Copyright 2022 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "util/compiler.h"
   #include "gallivm/lp_bld.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_struct.h"
   #include "gallivm/lp_bld_sample.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_ir_common.h"
   #include "draw/draw_vertex_header.h"
   #include "lp_bld_jit_types.h"
         static LLVMTypeRef
   lp_build_create_jit_buffer_type(struct gallivm_state *gallivm)
   {
      LLVMContextRef lc = gallivm->context;
   LLVMTypeRef buffer_type;
            elem_types[LP_JIT_BUFFER_BASE] = LLVMPointerType(LLVMInt32TypeInContext(lc), 0);
            buffer_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_buffer, f,
                  LP_CHECK_MEMBER_OFFSET(struct lp_jit_buffer, num_elements,
                  }
      LLVMValueRef
   lp_llvm_descriptor_base(struct gallivm_state *gallivm,
               {
               LLVMValueRef desc_set_index = LLVMBuildExtractValue(builder, index, 0, "");
   if (LLVMGetTypeKind(LLVMTypeOf(desc_set_index)) == LLVMVectorTypeKind)
                  LLVMValueRef binding_index = LLVMBuildExtractValue(builder, index, 1, "");
   if (LLVMGetTypeKind(LLVMTypeOf(binding_index)) == LLVMVectorTypeKind)
            LLVMValueRef binding_offset = LLVMBuildMul(builder, binding_index, lp_build_const_int32(gallivm, sizeof(struct lp_descriptor)), "");
   LLVMTypeRef int64_type = LLVMInt64TypeInContext(gallivm->context);
            LLVMValueRef desc_ptr = LLVMBuildPtrToInt(builder, desc_set_base, int64_type, "");
      }
      static LLVMValueRef
   lp_llvm_buffer_member(struct gallivm_state *gallivm,
                        LLVMValueRef buffers_ptr,
      {
                        LLVMValueRef ptr;
   if (LLVMGetTypeKind(LLVMTypeOf(buffers_offset)) == LLVMArrayTypeKind) {
               LLVMTypeRef buffer_ptr_type = LLVMPointerType(buffer_type, 0);
            LLVMValueRef indices[2] = {
      lp_build_const_int32(gallivm, 0),
      };
      } else {
               indices[0] = lp_build_const_int32(gallivm, 0);
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntULT, buffers_offset, lp_build_const_int32(gallivm, buffers_limit), "");
   indices[1] = LLVMBuildSelect(gallivm->builder, cond, buffers_offset, lp_build_const_int32(gallivm, 0), "");
            LLVMTypeRef buffers_type = LLVMArrayType(buffer_type, buffers_limit);
               LLVMTypeRef res_type = LLVMStructGetTypeAtIndex(buffer_type, member_index);
                        }
      LLVMValueRef
   lp_llvm_buffer_base(struct gallivm_state *gallivm,
         {
         }
      LLVMValueRef
   lp_llvm_buffer_num_elements(struct gallivm_state *gallivm,
         {
         }
      static LLVMTypeRef
   lp_build_create_jit_texture_type(struct gallivm_state *gallivm)
   {
      LLVMContextRef lc = gallivm->context;
   LLVMTypeRef texture_type;
            /* struct lp_jit_texture */
   elem_types[LP_JIT_TEXTURE_WIDTH]  =
   elem_types[LP_JIT_TEXTURE_HEIGHT] =
   elem_types[LP_JIT_TEXTURE_DEPTH] =
   elem_types[LP_JIT_TEXTURE_NUM_SAMPLES] =
   elem_types[LP_JIT_TEXTURE_SAMPLE_STRIDE] =
   elem_types[LP_JIT_TEXTURE_FIRST_LEVEL] =
   elem_types[LP_JIT_TEXTURE_LAST_LEVEL] = LLVMInt32TypeInContext(lc);
   elem_types[LP_JIT_TEXTURE_BASE] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
   elem_types[LP_JIT_TEXTURE_ROW_STRIDE] =
   elem_types[LP_JIT_TEXTURE_IMG_STRIDE] =
   elem_types[LP_JIT_TEXTURE_MIP_OFFSETS] =
            texture_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, width,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, height,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, depth,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, base,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, row_stride,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, img_stride,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, first_level,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, last_level,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, mip_offsets,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, num_samples,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_texture, sample_stride,
               LP_CHECK_STRUCT_SIZE(struct lp_jit_texture,
            }
      static LLVMTypeRef
   lp_build_create_jit_sampler_type(struct gallivm_state *gallivm)
   {
      LLVMContextRef lc = gallivm->context;
   LLVMTypeRef sampler_type;
   LLVMTypeRef elem_types[LP_JIT_SAMPLER_NUM_FIELDS];
   elem_types[LP_JIT_SAMPLER_MIN_LOD] =
   elem_types[LP_JIT_SAMPLER_MAX_LOD] =
   elem_types[LP_JIT_SAMPLER_LOD_BIAS] =
   elem_types[LP_JIT_SAMPLER_MAX_ANISO] = LLVMFloatTypeInContext(lc);
   elem_types[LP_JIT_SAMPLER_BORDER_COLOR] =
            sampler_type = LLVMStructTypeInContext(lc, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_sampler, min_lod,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_sampler, max_lod,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_sampler, lod_bias,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_sampler, border_color,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_sampler, max_aniso,
               LP_CHECK_STRUCT_SIZE(struct lp_jit_sampler,
            }
      static LLVMTypeRef
   lp_build_create_jit_image_type(struct gallivm_state *gallivm)
   {
      LLVMContextRef lc = gallivm->context;
   LLVMTypeRef image_type;
   LLVMTypeRef elem_types[LP_JIT_IMAGE_NUM_FIELDS];
   elem_types[LP_JIT_IMAGE_WIDTH] =
   elem_types[LP_JIT_IMAGE_HEIGHT] =
   elem_types[LP_JIT_IMAGE_DEPTH] = LLVMInt32TypeInContext(lc);
   elem_types[LP_JIT_IMAGE_BASE] = LLVMPointerType(LLVMInt8TypeInContext(lc), 0);
   elem_types[LP_JIT_IMAGE_ROW_STRIDE] =
   elem_types[LP_JIT_IMAGE_IMG_STRIDE] =
   elem_types[LP_JIT_IMAGE_NUM_SAMPLES] =
            image_type = LLVMStructTypeInContext(lc, elem_types,
         LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, width,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, height,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, depth,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, base,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, row_stride,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, img_stride,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, num_samples,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_image, sample_stride,
                  }
      LLVMTypeRef
   lp_build_jit_resources_type(struct gallivm_state *gallivm)
   {
      LLVMTypeRef elem_types[LP_JIT_RES_COUNT];
   LLVMTypeRef resources_type;
            buffer_type = lp_build_create_jit_buffer_type(gallivm);
   texture_type = lp_build_create_jit_texture_type(gallivm);
   sampler_type = lp_build_create_jit_sampler_type(gallivm);
   image_type = lp_build_create_jit_image_type(gallivm);
   elem_types[LP_JIT_RES_CONSTANTS] = LLVMArrayType(buffer_type,
         elem_types[LP_JIT_RES_SSBOS] =
         elem_types[LP_JIT_RES_TEXTURES] = LLVMArrayType(texture_type,
         elem_types[LP_JIT_RES_SAMPLERS] = LLVMArrayType(sampler_type,
         elem_types[LP_JIT_RES_IMAGES] = LLVMArrayType(image_type,
                  resources_type = LLVMStructTypeInContext(gallivm->context, elem_types,
            LP_CHECK_MEMBER_OFFSET(struct lp_jit_resources, constants,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_resources, ssbos,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_resources, textures,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_resources, samplers,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_resources, images,
               LP_CHECK_MEMBER_OFFSET(struct lp_jit_resources, aniso_filter_table,
                     }
         /**
   * Fetch the specified member of the lp_jit_texture structure.
   * \param emit_load  if TRUE, emit the LLVM load instruction to actually
   *                   fetch the field's value.  Otherwise, just emit the
   *                   GEP code to address the field.
   *
   * @sa http://llvm.org/docs/GetElementPtr.html
   */
   static LLVMValueRef
   lp_build_llvm_texture_member(struct gallivm_state *gallivm,
                              LLVMTypeRef resources_type,
   LLVMValueRef resources_ptr,
   unsigned texture_unit,
      {
               LLVMValueRef ptr;
   if (gallivm->texture_descriptor) {
      static_assert(offsetof(struct lp_descriptor, texture) == 0, "Invalid texture offset!");
            LLVMTypeRef texture_ptr_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_TEXTURES);
   LLVMTypeRef texture_type = LLVMGetElementType(texture_ptr_type);
                     LLVMValueRef indices[2] = {
      lp_build_const_int32(gallivm, 0),
      };
      } else {
                        /* resources[0] */
   indices[0] = lp_build_const_int32(gallivm, 0);
   /* resources[0].textures */
   indices[1] = lp_build_const_int32(gallivm, LP_JIT_RES_TEXTURES);
   /* resources[0].textures[unit] */
   indices[2] = lp_build_const_int32(gallivm, texture_unit);
   if (texture_unit_offset) {
      indices[2] = LLVMBuildAdd(gallivm->builder, indices[2],
         LLVMValueRef cond =
      LLVMBuildICmp(gallivm->builder, LLVMIntULT,
                  indices[2] = LLVMBuildSelect(gallivm->builder, cond, indices[2],
            }
   /* resources[0].textures[unit].member */
                        LLVMValueRef res;
   if (emit_load) {
      LLVMTypeRef tex_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_TEXTURES);
   LLVMTypeRef res_type = LLVMStructGetTypeAtIndex(LLVMGetElementType(tex_type), member_index);
      } else
            if (out_type) {
      LLVMTypeRef tex_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_TEXTURES);
   LLVMTypeRef res_type = LLVMStructGetTypeAtIndex(LLVMGetElementType(tex_type), member_index);
                           }
         /**
   * Helper macro to instantiate the functions that generate the code to
   * fetch the members of lp_jit_texture to fulfill the sampler code
   * generator requests.
   *
   * This complexity is the price we have to pay to keep the texture
   * sampler code generator a reusable module without dependencies to
   * llvmpipe internals.
   */
   #define LP_BUILD_LLVM_TEXTURE_MEMBER(_name, _index, _emit_load)  \
      static LLVMValueRef \
   lp_build_llvm_texture_##_name(struct gallivm_state *gallivm, \
                           { \
      return lp_build_llvm_texture_member(gallivm, resources_type, resources_ptr, \
                  #define LP_BUILD_LLVM_TEXTURE_MEMBER_OUTTYPE(_name, _index, _emit_load)  \
      static LLVMValueRef \
   lp_build_llvm_texture_##_name(struct gallivm_state *gallivm, \
                           LLVMTypeRef resources_type, \
   { \
      return lp_build_llvm_texture_member(gallivm, resources_type, resources_ptr, \
                  LP_BUILD_LLVM_TEXTURE_MEMBER(width,      LP_JIT_TEXTURE_WIDTH, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER(height,     LP_JIT_TEXTURE_HEIGHT, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER(depth,      LP_JIT_TEXTURE_DEPTH, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER(first_level, LP_JIT_TEXTURE_FIRST_LEVEL, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER(last_level, LP_JIT_TEXTURE_LAST_LEVEL, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER(base_ptr,   LP_JIT_TEXTURE_BASE, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER_OUTTYPE(row_stride, LP_JIT_TEXTURE_ROW_STRIDE, false)
   LP_BUILD_LLVM_TEXTURE_MEMBER_OUTTYPE(img_stride, LP_JIT_TEXTURE_IMG_STRIDE, false)
   LP_BUILD_LLVM_TEXTURE_MEMBER_OUTTYPE(mip_offsets, LP_JIT_TEXTURE_MIP_OFFSETS, false)
   LP_BUILD_LLVM_TEXTURE_MEMBER(num_samples, LP_JIT_TEXTURE_NUM_SAMPLES, true)
   LP_BUILD_LLVM_TEXTURE_MEMBER(sample_stride, LP_JIT_TEXTURE_SAMPLE_STRIDE, true)
      /**
   * Fetch the specified member of the lp_jit_sampler structure.
   * \param emit_load  if TRUE, emit the LLVM load instruction to actually
   *                   fetch the field's value.  Otherwise, just emit the
   *                   GEP code to address the field.
   *
   * @sa http://llvm.org/docs/GetElementPtr.html
   */
   static LLVMValueRef
   lp_build_llvm_sampler_member(struct gallivm_state *gallivm,
                              LLVMTypeRef resources_type,
      {
               LLVMValueRef ptr;
   if (gallivm->sampler_descriptor) {
      LLVMValueRef sampler_offset = lp_build_const_int64(gallivm, offsetof(struct lp_descriptor, sampler));
            LLVMTypeRef sampler_ptr_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_SAMPLERS);
   LLVMTypeRef sampler_type = LLVMGetElementType(sampler_ptr_type);
                     LLVMValueRef indices[2] = {
      lp_build_const_int32(gallivm, 0),
      };
      } else {
                        /* resources[0] */
   indices[0] = lp_build_const_int32(gallivm, 0);
   /* resources[0].samplers */
   indices[1] = lp_build_const_int32(gallivm, LP_JIT_RES_SAMPLERS);
   /* resources[0].samplers[unit] */
   indices[2] = lp_build_const_int32(gallivm, sampler_unit);
   /* resources[0].samplers[unit].member */
                        LLVMValueRef res;
   if (emit_load) {
      LLVMTypeRef samp_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_SAMPLERS);
   LLVMTypeRef res_type = LLVMStructGetTypeAtIndex(LLVMGetElementType(samp_type), member_index);
      } else
                        }
         #define LP_BUILD_LLVM_SAMPLER_MEMBER(_name, _index, _emit_load)   \
      static LLVMValueRef                                            \
   lp_build_llvm_sampler_##_name( struct gallivm_state *gallivm,  \
                     {                                                              \
      return lp_build_llvm_sampler_member(gallivm, resources_type, resources_ptr, \
               LP_BUILD_LLVM_SAMPLER_MEMBER(min_lod,    LP_JIT_SAMPLER_MIN_LOD, true)
   LP_BUILD_LLVM_SAMPLER_MEMBER(max_lod,    LP_JIT_SAMPLER_MAX_LOD, true)
   LP_BUILD_LLVM_SAMPLER_MEMBER(lod_bias,   LP_JIT_SAMPLER_LOD_BIAS, true)
   LP_BUILD_LLVM_SAMPLER_MEMBER(border_color, LP_JIT_SAMPLER_BORDER_COLOR, false)
   LP_BUILD_LLVM_SAMPLER_MEMBER(max_aniso, LP_JIT_SAMPLER_MAX_ANISO, true)
      /**
   * Fetch the specified member of the lp_jit_image structure.
   * \param emit_load  if TRUE, emit the LLVM load instruction to actually
   *                   fetch the field's value.  Otherwise, just emit the
   *                   GEP code to address the field.
   *
   * @sa http://llvm.org/docs/GetElementPtr.html
   */
   static LLVMValueRef
   lp_build_llvm_image_member(struct gallivm_state *gallivm,
                              LLVMTypeRef resources_type,
   LLVMValueRef resources_ptr,
      {
               LLVMValueRef ptr;
   if (gallivm->texture_descriptor) {
      LLVMValueRef image_offset = lp_build_const_int64(gallivm, offsetof(struct lp_descriptor, image));
            LLVMTypeRef image_ptr_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_IMAGES);
   LLVMTypeRef image_type = LLVMGetElementType(image_ptr_type);
                     LLVMValueRef indices[2] = {
      lp_build_const_int32(gallivm, 0),
      };
      } else {
                        /* resources[0] */
   indices[0] = lp_build_const_int32(gallivm, 0);
   /* resources[0].images */
   indices[1] = lp_build_const_int32(gallivm, LP_JIT_RES_IMAGES);
   /* resources[0].images[unit] */
   indices[2] = lp_build_const_int32(gallivm, image_unit);
   if (image_unit_offset) {
      indices[2] = LLVMBuildAdd(gallivm->builder, indices[2], image_unit_offset, "");
   LLVMValueRef cond = LLVMBuildICmp(gallivm->builder, LLVMIntULT, indices[2], lp_build_const_int32(gallivm, PIPE_MAX_SHADER_IMAGES), "");
      }
   /* resources[0].images[unit].member */
                        LLVMValueRef res;
   if (emit_load) {
      LLVMTypeRef img_type = LLVMStructGetTypeAtIndex(resources_type, LP_JIT_RES_IMAGES);
   LLVMTypeRef res_type = LLVMStructGetTypeAtIndex(LLVMGetElementType(img_type), member_index);
      } else
                        }
      /**
   * Helper macro to instantiate the functions that generate the code to
   * fetch the members of lp_jit_image to fulfill the sampler code
   * generator requests.
   *
   * This complexity is the price we have to pay to keep the image
   * sampler code generator a reusable module without dependencies to
   * llvmpipe internals.
   */
   #define LP_BUILD_LLVM_IMAGE_MEMBER(_name, _index, _emit_load)  \
      static LLVMValueRef \
   lp_build_llvm_image_##_name( struct gallivm_state *gallivm,               \
                     { \
      return lp_build_llvm_image_member(gallivm, resources_type, resources_ptr, \
                  #define LP_BUILD_LLVM_IMAGE_MEMBER_OUTTYPE(_name, _index, _emit_load)  \
      static LLVMValueRef \
   lp_build_llvm_image_##_name( struct gallivm_state *gallivm,               \
                           { \
      assert(!out_type);                                                \
   return lp_build_llvm_image_member(gallivm, resources_type, resources_ptr,    \
                  LP_BUILD_LLVM_IMAGE_MEMBER(width,      LP_JIT_IMAGE_WIDTH, true)
   LP_BUILD_LLVM_IMAGE_MEMBER(height,     LP_JIT_IMAGE_HEIGHT, true)
   LP_BUILD_LLVM_IMAGE_MEMBER(depth,      LP_JIT_IMAGE_DEPTH, true)
   LP_BUILD_LLVM_IMAGE_MEMBER(base_ptr,   LP_JIT_IMAGE_BASE, true)
   LP_BUILD_LLVM_IMAGE_MEMBER_OUTTYPE(row_stride, LP_JIT_IMAGE_ROW_STRIDE, true)
   LP_BUILD_LLVM_IMAGE_MEMBER_OUTTYPE(img_stride, LP_JIT_IMAGE_IMG_STRIDE, true)
   LP_BUILD_LLVM_IMAGE_MEMBER(num_samples, LP_JIT_IMAGE_NUM_SAMPLES, true)
   LP_BUILD_LLVM_IMAGE_MEMBER(sample_stride, LP_JIT_IMAGE_SAMPLE_STRIDE, true)
      void
   lp_build_jit_fill_sampler_dynamic_state(struct lp_sampler_dynamic_state *state)
   {
      state->width = lp_build_llvm_texture_width;
   state->height = lp_build_llvm_texture_height;
   state->depth = lp_build_llvm_texture_depth;
   state->first_level = lp_build_llvm_texture_first_level;
   state->last_level = lp_build_llvm_texture_last_level;
   state->base_ptr = lp_build_llvm_texture_base_ptr;
   state->row_stride = lp_build_llvm_texture_row_stride;
   state->img_stride = lp_build_llvm_texture_img_stride;
   state->mip_offsets = lp_build_llvm_texture_mip_offsets;
   state->num_samples = lp_build_llvm_texture_num_samples;
            state->min_lod = lp_build_llvm_sampler_min_lod;
   state->max_lod = lp_build_llvm_sampler_max_lod;
   state->lod_bias = lp_build_llvm_sampler_lod_bias;
   state->border_color = lp_build_llvm_sampler_border_color;
      }
      void
   lp_build_jit_fill_image_dynamic_state(struct lp_sampler_dynamic_state *state)
   {
      state->width = lp_build_llvm_image_width;
            state->depth = lp_build_llvm_image_depth;
   state->base_ptr = lp_build_llvm_image_base_ptr;
   state->row_stride = lp_build_llvm_image_row_stride;
   state->img_stride = lp_build_llvm_image_img_stride;
   state->num_samples = lp_build_llvm_image_num_samples;
      }
      /**
   * Create LLVM type for struct vertex_header;
   */
   LLVMTypeRef
   lp_build_create_jit_vertex_header_type(struct gallivm_state *gallivm, int data_elems)
   {
      LLVMTargetDataRef target = gallivm->target;
   LLVMTypeRef elem_types[3];
   LLVMTypeRef vertex_header;
                     elem_types[LP_JIT_VERTEX_HEADER_VERTEX_ID]  = LLVMIntTypeInContext(gallivm->context, 32);
   elem_types[LP_JIT_VERTEX_HEADER_CLIP_POS]  = LLVMArrayType(LLVMFloatTypeInContext(gallivm->context), 4);
            vertex_header = LLVMStructTypeInContext(gallivm->context, elem_types,
            /* these are bit-fields and we can't take address of them
      LP_CHECK_MEMBER_OFFSET(struct vertex_header, clipmask,
   target, vertex_header,
   LP_JIT_VERTEX_HEADER_CLIPMASK);
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, edgeflag,
   target, vertex_header,
   LP_JIT_VERTEX_HEADER_EDGEFLAG);
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, pad,
   target, vertex_header,
   LP_JIT_VERTEX_HEADER_PAD);
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, vertex_id,
   target, vertex_header,
      */
   (void) target; /* silence unused var warning for non-debug build */
   LP_CHECK_MEMBER_OFFSET(struct vertex_header, clip_pos,
               LP_CHECK_MEMBER_OFFSET(struct vertex_header, data,
                  assert(LLVMABISizeOfType(target, vertex_header) ==
               }
      LLVMTypeRef
   lp_build_sample_function_type(struct gallivm_state *gallivm, uint32_t sample_key)
   {
      struct lp_type type;
   memset(&type, 0, sizeof type);
   type.floating = true;      /* floating point values */
   type.sign = true;          /* values are signed */
   type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   type.width = 32;           /* 32-bit float */
            enum lp_sampler_op_type op_type = (sample_key & LP_SAMPLER_OP_TYPE_MASK) >> LP_SAMPLER_OP_TYPE_SHIFT;
            LLVMTypeRef arg_types[LP_MAX_TEX_FUNC_ARGS];
   LLVMTypeRef ret_type;
   LLVMTypeRef val_type[4];
            LLVMTypeRef coord_type;
   if (op_type == LP_SAMPLER_OP_FETCH)
         else
            arg_types[num_params++] = LLVMInt64TypeInContext(gallivm->context);
                     for (unsigned i = 0; i < 4; i++)
            if (sample_key & LP_SAMPLER_SHADOW)
            if (sample_key & LP_SAMPLER_FETCH_MS)
            if (sample_key & LP_SAMPLER_OFFSETS)
      for (uint32_t i = 0; i < 3; i++)
         if (lod_control == LP_SAMPLER_LOD_BIAS || lod_control == LP_SAMPLER_LOD_EXPLICIT)
            val_type[0] = val_type[1] = val_type[2] = val_type[3] = lp_build_vec_type(gallivm, type);
   ret_type = LLVMStructTypeInContext(gallivm->context, val_type, 4, 0);
      }
      LLVMTypeRef
   lp_build_size_function_type(struct gallivm_state *gallivm,
         {
      struct lp_type type;
   memset(&type, 0, sizeof type);
   type.floating = true;      /* floating point values */
   type.sign = true;          /* values are signed */
   type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   type.width = 32;           /* 32-bit float */
            LLVMTypeRef arg_types[LP_MAX_TEX_FUNC_ARGS];
   LLVMTypeRef ret_type;
   LLVMTypeRef val_type[4];
                     if (!params->samples_only)
            val_type[0] = val_type[1] = val_type[2] = val_type[3] = lp_build_int_vec_type(gallivm, type);
   ret_type = LLVMStructTypeInContext(gallivm->context, val_type, 4, 0);
      }
      LLVMTypeRef
   lp_build_image_function_type(struct gallivm_state *gallivm,
         {
      struct lp_type type;
   memset(&type, 0, sizeof type);
   type.floating = true;      /* floating point values */
   type.sign = true;          /* values are signed */
   type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   type.width = 32;           /* 32-bit float */
            LLVMTypeRef arg_types[LP_MAX_TEX_FUNC_ARGS];
   LLVMTypeRef ret_type;
                     if (params->img_op != LP_IMG_LOAD)
            for (uint32_t i = 0; i < 3; i++)
            if (ms)
            uint32_t num_inputs = params->img_op != LP_IMG_LOAD ? 4 : 0;
   if (params->img_op == LP_IMG_ATOMIC_CAS)
            const struct util_format_description *desc = util_format_description(params->format);
            for (uint32_t i = 0; i < num_inputs; i++)
            if (params->img_op != LP_IMG_STORE) {
      LLVMTypeRef val_type[4];
   val_type[0] = val_type[1] = val_type[2] = val_type[3] = component_type;
      } else  {
                     }
