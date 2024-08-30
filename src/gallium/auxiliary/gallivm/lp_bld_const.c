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
   * Helper functions for constant building.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include <float.h>
      #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/half_float.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_init.h"
   #include "lp_bld_limits.h"
         unsigned
   lp_mantissa(struct lp_type type)
   {
               if (type.floating) {
      switch (type.width) {
   case 16:
         case 32:
         case 64:
         default:
      assert(0);
         } else {
      if (type.sign)
         else
         }
         /**
   * Shift of the unity.
   *
   * Same as lp_const_scale(), but in terms of shifts.
   */
   unsigned
   lp_const_shift(struct lp_type type)
   {
      if (type.floating)
         else if (type.fixed)
         else if (type.norm)
         else
      }
         unsigned
   lp_const_offset(struct lp_type type)
   {
      if (type.floating || type.fixed)
         else if (type.norm)
         else
      }
         /**
   * Scaling factor between the LLVM native value and its interpretation.
   *
   * This is 1.0 for all floating types and unnormalized integers, and something
   * else for the fixed points types and normalized integers.
   */
   double
   lp_const_scale(struct lp_type type)
   {
      unsigned long long llscale;
            llscale = (unsigned long long)1 << lp_const_shift(type);
   llscale -= lp_const_offset(type);
   dscale = (double)llscale;
               }
         /**
   * Minimum value representable by the type.
   */
   double
   lp_const_min(struct lp_type type)
   {
      if (!type.sign)
            if (type.norm)
            if (type.floating) {
      switch (type.width) {
   case 16:
         case 32:
         case 64:
         default:
      assert(0);
                  unsigned bits;
   if (type.fixed)
      /* FIXME: consider the fractional bits? */
      else
               }
         /**
   * Maximum value representable by the type.
   */
   double
   lp_const_max(struct lp_type type)
   {
      if (type.norm)
            if (type.floating) {
      switch (type.width) {
   case 16:
         case 32:
         case 64:
         default:
      assert(0);
                  unsigned bits;
   if (type.fixed)
         else
            if (type.sign)
               }
         double
   lp_const_eps(struct lp_type type)
   {
      if (type.floating) {
      switch (type.width) {
   case 16:
         case 32:
         case 64:
         default:
      assert(0);
         } else {
      double scale = lp_const_scale(type);
         }
         LLVMValueRef
   lp_build_undef(struct gallivm_state *gallivm, struct lp_type type)
   {
      LLVMTypeRef vec_type = lp_build_vec_type(gallivm, type);
      }
         LLVMValueRef
   lp_build_zero(struct gallivm_state *gallivm, struct lp_type type)
   {
      if (type.length == 1) {
      if (type.floating)
         else
      } else {
      LLVMTypeRef vec_type = lp_build_vec_type(gallivm, type);
         }
         LLVMValueRef
   lp_build_one(struct gallivm_state *gallivm, struct lp_type type)
   {
      LLVMTypeRef elem_type;
                              if (!lp_has_fp16() && type.floating && type.width == 16)
         else if (type.floating)
         else if (type.fixed)
         else if (!type.norm)
         else if (type.sign)
         else {
      /* special case' -- 1.0 for normalized types is more easily attained if
   * we start with a vector consisting of all bits set */
   LLVMTypeRef vec_type = lp_build_vec_type(gallivm, type);
      #if 0
         if (type.sign)
      /* TODO: Unfortunately this caused "Tried to create a shift operation
      #endif
                        for (unsigned i = 1; i < type.length; ++i)
            if (type.length == 1)
         else
      }
         /**
   * Build constant-valued element from a scalar value.
   */
   LLVMValueRef
   lp_build_const_elem(struct gallivm_state *gallivm,
               {
      LLVMTypeRef elem_type = lp_build_elem_type(gallivm, type);
            if (!lp_has_fp16() && type.floating && type.width == 16) {
         } else if (type.floating) {
         } else {
                              }
         /**
   * Build constant-valued vector from a scalar value.
   */
   LLVMValueRef
   lp_build_const_vec(struct gallivm_state *gallivm, struct lp_type type,
         {
      if (type.length == 1) {
         } else {
      LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   elems[0] = lp_build_const_elem(gallivm, type, val);
   for (unsigned i = 1; i < type.length; ++i)
               }
         LLVMValueRef
   lp_build_const_int_vec(struct gallivm_state *gallivm, struct lp_type type,
         {
      LLVMTypeRef elem_type = lp_build_int_elem_type(gallivm, type);
                     for (unsigned i = 0; i < type.length; ++i)
            if (type.length == 1)
               }
      /* Returns an integer vector of [0, 1, 2, ...] */
   LLVMValueRef
   lp_build_const_channel_vec(struct gallivm_state *gallivm, struct lp_type type)
   {
      LLVMTypeRef elem_type = lp_build_int_elem_type(gallivm, type);
                     for (unsigned i = 0; i < type.length; ++i)
            if (type.length == 1)
               }
         LLVMValueRef
   lp_build_const_aos(struct gallivm_state *gallivm,
                     {
      const unsigned char default_swizzle[4] = {0, 1, 2, 3};
            assert(type.length % 4 == 0);
                     if (!swizzle)
            elems[swizzle[0]] = lp_build_const_elem(gallivm, type, r);
   elems[swizzle[1]] = lp_build_const_elem(gallivm, type, g);
   elems[swizzle[2]] = lp_build_const_elem(gallivm, type, b);
            for (unsigned i = 4; i < type.length; ++i)
               }
         /**
   * @param mask TGSI_WRITEMASK_xxx
   */
   LLVMValueRef
   lp_build_const_mask_aos(struct gallivm_state *gallivm,
                     {
      LLVMTypeRef elem_type = LLVMIntTypeInContext(gallivm->context, type.width);
                     for (unsigned j = 0; j < type.length; j += channels) {
      for (unsigned i = 0; i < channels; ++i) {
      masks[j + i] = LLVMConstInt(elem_type,
                           }
         /**
   * Performs lp_build_const_mask_aos, but first swizzles the mask
   */
   LLVMValueRef
   lp_build_const_mask_aos_swizzled(struct gallivm_state *gallivm,
                           {
               for (unsigned i = 0; i < channels; ++i) {
      if (swizzle[i] < 4) {
                        }
         /**
   * Build a zero-terminated constant string.
   */
   LLVMValueRef
   lp_build_const_string(struct gallivm_state *gallivm,
         {
      unsigned len = strlen(str) + 1;
   LLVMTypeRef i8 = LLVMInt8TypeInContext(gallivm->context);
   LLVMValueRef string = LLVMAddGlobal(gallivm->module, LLVMArrayType(i8, len), "");
   LLVMSetGlobalConstant(string, true);
   LLVMSetLinkage(string, LLVMInternalLinkage);
   LLVMSetInitializer(string, LLVMConstStringInContext(gallivm->context, str, len, true));
   string = LLVMConstBitCast(string, LLVMPointerType(i8, 0));
      }
         LLVMValueRef
   lp_build_const_func_pointer_from_type(struct gallivm_state *gallivm,
                     {
      return LLVMBuildBitCast(gallivm->builder,
                  }
         /**
   * Build a callable function pointer.
   *
   * We use function pointer constants instead of LLVMAddGlobalMapping()
   * to work around a bug in LLVM 2.6, and for efficiency/simplicity.
   */
   LLVMValueRef
   lp_build_const_func_pointer(struct gallivm_state *gallivm,
                                 {
      LLVMTypeRef function_type = LLVMFunctionType(ret_type, arg_types, num_args, 0);
      }
