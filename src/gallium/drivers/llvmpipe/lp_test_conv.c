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
   * Unit tests for type conversion.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/u_pointer.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_type.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_conv.h"
   #include "gallivm/lp_bld_debug.h"
   #include "lp_test.h"
         typedef void (*conv_test_ptr_t)(const void *src, const void *dst);
         void
   write_tsv_header(FILE *fp)
   {
      fprintf(fp,
         "result\t"
   "cycles_per_channel\t"
               }
         static void
   write_tsv_row(FILE *fp,
               struct lp_type src_type,
   struct lp_type dst_type,
   {
                        dump_type(fp, src_type);
            dump_type(fp, dst_type);
               }
         static void
   dump_conv_types(FILE *fp,
               {
      fprintf(fp, "src_type=");
            fprintf(fp, " dst_type=");
            fprintf(fp, " ...\n");
      }
         static LLVMValueRef
   add_conv_test(struct gallivm_state *gallivm,
               {
      LLVMModuleRef module = gallivm->module;
   LLVMContextRef context = gallivm->context;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef args[2];
   LLVMValueRef func;
   LLVMValueRef src_ptr;
   LLVMValueRef dst_ptr;
   LLVMBasicBlockRef block;
   LLVMValueRef src[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef dst[LP_MAX_VECTOR_LENGTH];
   unsigned i;
   LLVMTypeRef src_vec_type = lp_build_vec_type(gallivm, src_type);
            args[0] = LLVMPointerType(src_vec_type, 0);
            func = LLVMAddFunction(module, "test",
               LLVMSetFunctionCallConv(func, LLVMCCallConv);
   src_ptr = LLVMGetParam(func, 0);
            block = LLVMAppendBasicBlockInContext(context, func, "entry");
            for (i = 0; i < num_srcs; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32TypeInContext(context), i, 0);
   LLVMValueRef ptr = LLVMBuildGEP2(builder, src_vec_type, src_ptr, &index, 1, "");
                        for (i = 0; i < num_dsts; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32TypeInContext(context), i, 0);
   LLVMValueRef ptr = LLVMBuildGEP2(builder, dst_vec_type, dst_ptr, &index, 1, "");
                                    }
         UTIL_ALIGN_STACK
   static bool
   test_one(unsigned verbose,
            FILE *fp,
      {
      LLVMContextRef context;
   struct gallivm_state *gallivm;
   LLVMValueRef func = NULL;
   conv_test_ptr_t conv_test_ptr;
   bool success;
   const unsigned n = LP_TEST_NUM_SAMPLES;
   int64_t cycles[LP_TEST_NUM_SAMPLES];
   double cycles_avg = 0.0;
   unsigned num_srcs;
   unsigned num_dsts;
   double eps;
            if ((src_type.width >= dst_type.width && src_type.length > dst_type.length) ||
      (src_type.width <= dst_type.width && src_type.length < dst_type.length)) {
               /* Known failures
   * - fixed point 32 -> float 32
   * - float 32 -> signed normalized integer 32
   */
   if ((src_type.floating && !dst_type.floating && dst_type.sign && dst_type.norm && src_type.width == dst_type.width) ||
      (!src_type.floating && dst_type.floating && src_type.fixed && src_type.width == dst_type.width)) {
               /* Known failures
   * - fixed point 32 -> float 32
   * - float 32 -> signed normalized integer 32
   */
   if ((src_type.floating && !dst_type.floating && dst_type.sign && dst_type.norm && src_type.width == dst_type.width) ||
      (!src_type.floating && dst_type.floating && src_type.fixed && src_type.width == dst_type.width)) {
               if (verbose >= 1)
            if (src_type.length > dst_type.length) {
      num_srcs = 1;
      }
   else if (src_type.length < dst_type.length) {
      num_dsts = 1;
      }
   else  {
      num_dsts = 1;
               /* We must not loose or gain channels. Only precision */
            eps = MAX2(lp_const_eps(src_type), lp_const_eps(dst_type));
   if (dst_type.norm && dst_type.sign && src_type.sign && !src_type.floating) {
      /*
   * This is quite inaccurate due to shift being used.
   * I don't think it's possible to hit such conversions with
   * llvmpipe though.
   */
                  #if LLVM_VERSION_MAJOR == 15
         #endif
                                                   success = true;
   for (i = 0; i < n && success; ++i) {
      unsigned src_stride = src_type.length*src_type.width/8;
   unsigned dst_stride = dst_type.length*dst_type.width/8;
   alignas(LP_MIN_VECTOR_ALIGN) uint8_t src[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
   alignas(LP_MIN_VECTOR_ALIGN) uint8_t dst[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
   double fref[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
   uint8_t ref[LP_MAX_VECTOR_LENGTH*LP_MAX_VECTOR_LENGTH];
   int64_t start_counter = 0;
            for (j = 0; j < num_srcs; ++j) {
      random_vec(src_type, src + j*src_stride);
               for (j = 0; j < num_dsts; ++j) {
                  start_counter = rdtsc();
   conv_test_ptr(src, dst);
                     for (j = 0; j < num_dsts; ++j) {
      if (!compare_vec_with_eps(dst_type, dst + j*dst_stride, ref + j*dst_stride, eps))
               if (!success || verbose >= 3) {
      if (verbose < 1)
         if (success) {
         }
   else {
                  for (j = 0; j < num_srcs; ++j) {
      fprintf(stderr, "  Src%u: ", j);
   dump_vec(stderr, src_type, src + j*src_stride);
      #if 1
            fprintf(stderr, "  Ref: ");
   for (j = 0; j < src_type.length*num_srcs; ++j)
      #endif
               for (j = 0; j < num_dsts; ++j) {
      fprintf(stderr, "  Dst%u: ", j);
                  fprintf(stderr, "  Ref%u: ", j);
   dump_vec(stderr, dst_type, ref + j*dst_stride);
                     /*
   * Unfortunately the output of cycle counter is not very reliable as it comes
   * -- sometimes we get outliers (due IRQs perhaps?) which are
   * better removed to avoid random or biased data.
   */
   {
      double sum = 0.0, sum2 = 0.0;
   double avg, std;
            for (i = 0; i < n; ++i) {
      sum += cycles[i];
               avg = sum/n;
            m = 0;
   sum = 0.0;
   for (i = 0; i < n; ++i) {
      if (fabs(cycles[i] - avg) <= 4.0*std) {
      sum += cycles[i];
                                 if (fp)
            gallivm_destroy(gallivm);
               }
         const struct lp_type conv_types[] = {
               /* Float */
   {   true, false,  true,  true,    32,   4 },
   {   true, false,  true, false,    32,   4 },
   {   true, false, false,  true,    32,   4 },
            {   true, false,  true,  true,    32,   8 },
   {   true, false,  true, false,    32,   8 },
   {   true, false, false,  true,    32,   8 },
            /* Fixed */
   {  false,  true,  true,  true,    32,   4 },
   {  false,  true,  true, false,    32,   4 },
   {  false,  true, false,  true,    32,   4 },
            {  false,  true,  true,  true,    32,   8 },
   {  false,  true,  true, false,    32,   8 },
   {  false,  true, false,  true,    32,   8 },
            /* Integer */
   {  false, false,  true,  true,    32,   4 },
   {  false, false,  true, false,    32,   4 },
   {  false, false, false,  true,    32,   4 },
            {  false, false,  true,  true,    32,   8 },
   {  false, false,  true, false,    32,   8 },
   {  false, false, false,  true,    32,   8 },
            {  false, false,  true,  true,    16,   8 },
   {  false, false,  true, false,    16,   8 },
   {  false, false, false,  true,    16,   8 },
            {  false, false,  true,  true,     8,  16 },
   {  false, false,  true, false,     8,  16 },
   {  false, false, false,  true,     8,  16 },
            {  false, false,  true,  true,     8,   4 },
   {  false, false,  true, false,     8,   4 },
   {  false, false, false,  true,     8,   4 },
               };
         const unsigned num_types = ARRAY_SIZE(conv_types);
         bool
   test_all(unsigned verbose, FILE *fp)
   {
      const struct lp_type *src_type;
   const struct lp_type *dst_type;
   bool success = true;
            for (src_type = conv_types; src_type < &conv_types[num_types]; ++src_type) {
                                 if (!test_one(verbose, fp, *src_type, *dst_type)){
      success = false;
                                 }
         bool
   test_some(unsigned verbose, FILE *fp,
         {
      const struct lp_type *src_type;
   const struct lp_type *dst_type;
   unsigned long i;
            for (i = 0; i < n; ++i) {
               do {
                  if (!test_one(verbose, fp, *src_type, *dst_type))
                  }
         bool
   test_single(unsigned verbose, FILE *fp)
   {
      /*    float, fixed,  sign,  norm, width, len */
   struct lp_type f32x4_type =
         struct lp_type ub8x4_type =
                                 }
