   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
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
         /**
   * @file
   * Helpers for emiting intrinsic calls.
   *
   * LLVM vanilla IR doesn't represent all basic arithmetic operations we care
   * about, and it is often necessary to resort target-specific intrinsics for
   * performance, convenience.
   *
   * Ideally we would like to stay away from target specific intrinsics and
   * move all the instruction selection logic into upstream LLVM where it belongs.
   *
   * These functions are also used for calling C functions provided by us from
   * generated LLVM code.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include <llvm/Config/llvm-config.h>
      #include "util/u_debug.h"
   #include "util/u_string.h"
   #include "util/bitscan.h"
      #include "lp_bld_const.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_type.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_debug.h"
         void
   lp_format_intrinsic(char *name,
                     {
      unsigned length = 0;
   unsigned width;
            LLVMTypeKind kind = LLVMGetTypeKind(type);
   if (kind == LLVMVectorTypeKind) {
      length = LLVMGetVectorSize(type);
   type = LLVMGetElementType(type);
               switch (kind) {
   case LLVMIntegerTypeKind:
      c = 'i';
   width = LLVMGetIntTypeWidth(type);
      case LLVMFloatTypeKind:
      c = 'f';
   width = 32;
      case LLVMDoubleTypeKind:
      c = 'f';
   width = 64;
      case LLVMHalfTypeKind:
      c = 'f';
   width = 16;
      default:
                  if (length) {
         } else {
            }
         LLVMValueRef
   lp_declare_intrinsic_with_type(LLVMModuleRef module,
               {
                        LLVMSetFunctionCallConv(function, LLVMCCallConv);
                        }
         LLVMValueRef
   lp_declare_intrinsic(LLVMModuleRef module,
                           {
      LLVMTypeRef function_type = LLVMFunctionType(ret_type, arg_types, num_args, 0);
      }
      static const char *attr_to_str(enum lp_func_attr attr)
   {
      switch (attr) {
   case LP_FUNC_ATTR_ALWAYSINLINE: return "alwaysinline";
   case LP_FUNC_ATTR_INREG: return "inreg";
   case LP_FUNC_ATTR_NOALIAS: return "noalias";
   case LP_FUNC_ATTR_NOUNWIND: return "nounwind";
   case LP_FUNC_ATTR_CONVERGENT: return "convergent";
   case LP_FUNC_ATTR_PRESPLITCORO: return "presplitcoroutine";
   default:
      _debug_printf("Unhandled function attribute: %x\n", attr);
         }
      void
   lp_add_function_attr(LLVMValueRef function_or_call,
         {
      LLVMModuleRef module;
   if (LLVMIsAFunction(function_or_call)) {
         } else {
      LLVMBasicBlockRef bb = LLVMGetInstructionParent(function_or_call);
   LLVMValueRef function = LLVMGetBasicBlockParent(bb);
      }
            const char *attr_name = attr_to_str(attr);
   unsigned kind_id = LLVMGetEnumAttributeKindForName(attr_name,
                  if (LLVMIsAFunction(function_or_call))
         else
      }
      static void
   lp_add_func_attributes(LLVMValueRef function, unsigned attrib_mask)
   {
      /* NoUnwind indicates that the intrinsic never raises a C++ exception.
   * Set it for all intrinsics.
   */
            while (attrib_mask) {
      enum lp_func_attr attr = 1u << u_bit_scan(&attrib_mask);
         }
      LLVMValueRef
   lp_build_intrinsic(LLVMBuilderRef builder,
                     const char *name,
   LLVMTypeRef ret_type,
   {
      LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
                              for(unsigned i = 0; i < num_args; ++i) {
      assert(args[i]);
                                 if(!function) {
               /*
   * If llvm removes an intrinsic we use, we'll hit this abort (rather
   * than a call to address zero in the jited code).
   */
   if (LLVMGetIntrinsicID(function) == 0) {
      _debug_printf("llvm (version " MESA_LLVM_VERSION_STRING
                           if (gallivm_debug & GALLIVM_DEBUG_IR) {
                     call = LLVMBuildCall2(builder, function_type, function, args, num_args, "");
   lp_add_func_attributes(call, attr_mask);
      }
         LLVMValueRef
   lp_build_intrinsic_unary(LLVMBuilderRef builder,
                     {
         }
         LLVMValueRef
   lp_build_intrinsic_binary(LLVMBuilderRef builder,
                           {
               args[0] = a;
               }
         /**
   * Call intrinsic with arguments adapted to intrinsic vector length.
   *
   * Split vectors which are too large for the hw, or expand them if they
   * are too small, so a caller calling a function which might use intrinsics
   * doesn't need to do splitting/expansion on its own.
   * This only supports intrinsics where src and dst types match.
   */
   LLVMValueRef
   lp_build_intrinsic_binary_anylength(struct gallivm_state *gallivm,
                                 {
      unsigned i;
   struct lp_type intrin_type = src_type;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef i32undef = LLVMGetUndef(LLVMInt32TypeInContext(gallivm->context));
   LLVMValueRef anative, bnative;
                     if (intrin_length > src_type.length) {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
            for (i = 0; i < src_type.length; i++) {
         }
   for (; i < intrin_length; i++) {
         }
   if (src_type.length == 1) {
      LLVMTypeRef elem_type = lp_build_elem_type(gallivm, intrin_type);
   a = LLVMBuildBitCast(builder, a, LLVMVectorType(elem_type, 1), "");
      }
   constvec = LLVMConstVector(elems, intrin_length);
   anative = LLVMBuildShuffleVector(builder, a, a, constvec, "");
   bnative = LLVMBuildShuffleVector(builder, b, b, constvec, "");
   tmp = lp_build_intrinsic_binary(builder, name,
               if (src_type.length > 1) {
      constvec = LLVMConstVector(elems, src_type.length);
      }
   else {
            }
   else if (intrin_length < src_type.length) {
      unsigned num_vec = src_type.length / intrin_length;
            /* don't support arbitrary size here as this is so yuck */
   if (src_type.length % intrin_length) {
      /* FIXME: This is something which should be supported
   * but there doesn't seem to be any need for it currently
   * so crash and burn.
   */
   debug_printf("%s: should handle arbitrary vector size\n",
         assert(0);
               for (i = 0; i < num_vec; i++) {
      anative = lp_build_extract_range(gallivm, a, i*intrin_length,
         bnative = lp_build_extract_range(gallivm, b, i*intrin_length,
         tmp[i] = lp_build_intrinsic_binary(builder, name,
            }
      }
   else {
      return lp_build_intrinsic_binary(builder, name,
               }
         LLVMValueRef
   lp_build_intrinsic_map(struct gallivm_state *gallivm,
                           {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef ret_elem_type = LLVMGetElementType(ret_type);
   unsigned n = LLVMGetVectorSize(ret_type);
   unsigned i, j;
                     res = LLVMGetUndef(ret_type);
   for(i = 0; i < n; ++i) {
      LLVMValueRef index = lp_build_const_int32(gallivm, i);
   LLVMValueRef arg_elems[LP_MAX_FUNC_ARGS];
   LLVMValueRef res_elem;
   for(j = 0; j < num_args; ++j)
         res_elem = lp_build_intrinsic(builder, name, ret_elem_type, arg_elems, num_args, 0);
                  }
         LLVMValueRef
   lp_build_intrinsic_map_unary(struct gallivm_state *gallivm,
                     {
         }
         LLVMValueRef
   lp_build_intrinsic_map_binary(struct gallivm_state *gallivm,
                           {
               args[0] = a;
               }
      