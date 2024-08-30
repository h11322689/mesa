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
         #include "util/u_debug.h"
      #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_init.h"
   #include "lp_bld_limits.h"
         /*
   * Return a scalar LLVMTypeRef corresponding to the given lp_type.
   */
   LLVMTypeRef
   lp_build_elem_type(const struct gallivm_state *gallivm, struct lp_type type)
   {
      if (type.floating) {
      switch (type.width) {
   case 16:
      return lp_has_fp16()
      ? LLVMHalfTypeInContext(gallivm->context)
   case 32:
         case 64:
         default:
      assert(0);
         }
   else {
            }
         /*
   * Return a vector LLVMTypeRef corresponding to the given lp_type.
   */
   LLVMTypeRef
   lp_build_vec_type(const struct gallivm_state *gallivm, struct lp_type type)
   {
      LLVMTypeRef elem_type = lp_build_elem_type(gallivm, type);
   if (type.length == 1)
         else
      }
         /**
   * This function is a mirror of lp_build_elem_type() above.
   *
   * XXX: I'm not sure if it wouldn't be easier/efficient to just recreate the
   * type and check for identity.
   */
   bool
   lp_check_elem_type(struct lp_type type, LLVMTypeRef elem_type)
   {
      assert(elem_type);
   if (!elem_type)
                     if (type.floating) {
      switch (type.width) {
   case 16:
      if (elem_kind != (lp_has_fp16()
            debug_printf("%s:%d: type is not 16 bits\n", __FILE__, __LINE__);
      }
      case 32:
      if (elem_kind != LLVMFloatTypeKind) {
      debug_printf("%s:%d: type is not float\n", __FILE__, __LINE__);
      }
      case 64:
      if (elem_kind != LLVMDoubleTypeKind) {
      debug_printf("%s:%d: type is not double\n", __FILE__, __LINE__);
      }
      default:
      assert(0);
         }
   else {
      if (elem_kind != LLVMIntegerTypeKind) {
      debug_printf("%s:%d: element is not integer\n", __FILE__, __LINE__);
               if (LLVMGetIntTypeWidth(elem_type) != type.width) {
      debug_printf("%s:%d: type width mismatch %d != %d\n",
                                 }
         bool
   lp_check_vec_type(struct lp_type type, LLVMTypeRef vec_type)
   {
      assert(vec_type);
   if (!vec_type)
            if (type.length == 1)
            if (LLVMGetTypeKind(vec_type) != LLVMVectorTypeKind) {
      printf("%s:%d: kind is not vector\n", __FILE__, __LINE__);
               if (LLVMGetVectorSize(vec_type) != type.length) {
      printf("%s:%d: vector size mismatch %d != expected %d\n", __FILE__, __LINE__,
                                 }
         bool
   lp_check_value(struct lp_type type, LLVMValueRef val)
   {
      assert(val);
   if (!val)
                        }
         LLVMTypeRef
   lp_build_int_elem_type(const struct gallivm_state *gallivm,
         {
         }
         LLVMTypeRef
   lp_build_int_vec_type(const struct gallivm_state *gallivm,
         {
      LLVMTypeRef elem_type = lp_build_int_elem_type(gallivm, type);
   if (type.length == 1)
         else
      }
         /**
   * Create element of vector type
   */
   struct lp_type
   lp_elem_type(struct lp_type type)
   {
               assert(type.length > 1);
   res_type = type;
               }
         /**
   * Create unsigned integer type variation of given type.
   */
   struct lp_type
   lp_uint_type(struct lp_type type)
   {
               assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
               }
         /**
   * Create signed integer type variation of given type.
   */
   struct lp_type
   lp_int_type(struct lp_type type)
   {
               assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
   res_type.length = type.length;
               }
         /**
   * Return the type with twice the bit width (hence half the number of elements).
   */
   struct lp_type
   lp_wider_type(struct lp_type type)
   {
               memcpy(&res_type, &type, sizeof res_type);
   res_type.width *= 2;
                        }
         /**
   * Return the size of the LLVMType in bits.
   * XXX this function doesn't necessarily handle all LLVM types.
   */
   unsigned
   lp_sizeof_llvm_type(LLVMTypeRef t)
   {
               switch (k) {
   case LLVMIntegerTypeKind:
         case LLVMFloatTypeKind:
         case LLVMDoubleTypeKind:
         case LLVMHalfTypeKind:
         case LLVMVectorTypeKind:
      {
      LLVMTypeRef elem = LLVMGetElementType(t);
   unsigned len = LLVMGetVectorSize(t);
      }
      case LLVMArrayTypeKind:
      {
      LLVMTypeRef elem = LLVMGetElementType(t);
   unsigned len = LLVMGetArrayLength(t);
      }
      default:
      assert(0 && "Unexpected type in lp_get_llvm_type_size()");
         }
         /**
   * Return string name for a LLVMTypeKind.  Useful for debugging.
   */
   const char *
   lp_typekind_name(LLVMTypeKind t)
   {
      switch (t) {
   case LLVMVoidTypeKind:
         case LLVMFloatTypeKind:
         case LLVMHalfTypeKind:
         case LLVMDoubleTypeKind:
         case LLVMX86_FP80TypeKind:
         case LLVMFP128TypeKind:
         case LLVMPPC_FP128TypeKind:
         case LLVMLabelTypeKind:
         case LLVMIntegerTypeKind:
         case LLVMFunctionTypeKind:
         case LLVMStructTypeKind:
         case LLVMArrayTypeKind:
         case LLVMPointerTypeKind:
         case LLVMVectorTypeKind:
         case LLVMMetadataTypeKind:
         default:
            }
         /**
   * Print an LLVMTypeRef.  Like LLVMDumpValue().  For debugging.
   */
   void
   lp_dump_llvmtype(LLVMTypeRef t)
   {
               if (k == LLVMVectorTypeKind) {
      LLVMTypeRef te = LLVMGetElementType(t);
   LLVMTypeKind ke = LLVMGetTypeKind(te);
   unsigned len = LLVMGetVectorSize(t);
   if (ke == LLVMIntegerTypeKind) {
      unsigned b = LLVMGetIntTypeWidth(te);
      }
   else {
            }
   else if (k == LLVMArrayTypeKind) {
      LLVMTypeRef te = LLVMGetElementType(t);
   LLVMTypeKind ke = LLVMGetTypeKind(te);
   unsigned len = LLVMGetArrayLength(t);
      }
   else if (k == LLVMIntegerTypeKind) {
      unsigned b = LLVMGetIntTypeWidth(t);
      }
   else if (k == LLVMPointerTypeKind) {
      LLVMTypeRef te = LLVMGetElementType(t);
   debug_printf("Pointer to ");
      }
   else {
            }
         void
   lp_build_context_init(struct lp_build_context *bld,
               {
      bld->gallivm = gallivm;
            bld->int_elem_type = lp_build_int_elem_type(gallivm, type);
   if (type.floating)
         else
            if (type.length == 1) {
      bld->int_vec_type = bld->int_elem_type;
      }
   else {
      bld->int_vec_type = LLVMVectorType(bld->int_elem_type, type.length);
               bld->undef = LLVMGetUndef(bld->vec_type);
   bld->zero = LLVMConstNull(bld->vec_type);
      }
         /**
   * Count the number of instructions in a function.
   */
   static unsigned
   lp_build_count_instructions(LLVMValueRef function)
   {
      unsigned num_instrs = 0;
            block = LLVMGetFirstBasicBlock(function);
   while (block) {
      LLVMValueRef instr;
   instr = LLVMGetFirstInstruction(block);
   while (instr) {
                  }
                  }
         /**
   * Count the number of instructions in a module.
   */
   unsigned
   lp_build_count_ir_module(LLVMModuleRef module)
   {
      LLVMValueRef func;
            func = LLVMGetFirstFunction(module);
   while (func) {
      num_instrs += lp_build_count_instructions(func);
      }
      }
