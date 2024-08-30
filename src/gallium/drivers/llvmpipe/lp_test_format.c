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
         #include <stdlib.h>
   #include <stdio.h>
   #include <float.h>
      #include "util/u_memory.h"
   #include "util/u_pointer.h"
   #include "util/u_string.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_tests.h"
   #include "util/format/u_format_s3tc.h"
      #include "gallivm/lp_bld.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_format.h"
   #include "gallivm/lp_bld_init.h"
      #include "lp_test.h"
      static struct lp_build_format_cache *cache_ptr;
      void
   write_tsv_header(FILE *fp)
   {
      fprintf(fp,
                     }
         static void
   write_tsv_row(FILE *fp,
               {
                           }
         typedef void
   (*fetch_ptr_t)(void *unpacked, const void *packed,
               static LLVMValueRef
   add_fetch_rgba_test(struct gallivm_state *gallivm, unsigned verbose,
                     {
      char name[256];
   LLVMContextRef context = gallivm->context;
   LLVMModuleRef module = gallivm->module;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef args[5];
   LLVMValueRef func;
   LLVMValueRef packed_ptr;
   LLVMValueRef offset = LLVMConstNull(LLVMInt32TypeInContext(context));
   LLVMValueRef rgba_ptr;
   LLVMValueRef i;
   LLVMValueRef j;
   LLVMBasicBlockRef block;
   LLVMValueRef rgba;
            snprintf(name, sizeof name, "fetch_%s_%s", desc->short_name,
            args[0] = LLVMPointerType(lp_build_vec_type(gallivm, type), 0);
   args[1] = LLVMPointerType(LLVMInt8TypeInContext(context), 0);
   args[3] = args[2] = LLVMInt32TypeInContext(context);
            func = LLVMAddFunction(module, name,
               LLVMSetFunctionCallConv(func, LLVMCCallConv);
   rgba_ptr = LLVMGetParam(func, 0);
   packed_ptr = LLVMGetParam(func, 1);
   i = LLVMGetParam(func, 2);
            if (use_cache) {
                  block = LLVMAppendBasicBlockInContext(context, func, "entry");
            rgba = lp_build_fetch_rgba_aos(gallivm, desc, type, true,
                                          }
         UTIL_ALIGN_STACK
   static bool
   test_format_float(unsigned verbose, FILE *fp,
               {
      LLVMContextRef context;
   struct gallivm_state *gallivm;
   LLVMValueRef fetch = NULL;
   fetch_ptr_t fetch_ptr;
   alignas(16) uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   alignas(16) float unpacked[4];
   bool first = true;
   bool success = true;
               #if LLVM_VERSION_MAJOR == 15
         #endif
               fetch = add_fetch_rgba_test(gallivm, verbose, desc,
                                       for (l = 0; l < util_format_nr_test_cases; ++l) {
                           if (first) {
      printf("Testing %s (float) ...\n",
         fflush(stdout);
                              for (i = 0; i < desc->block.height; ++i) {
                                          for (k = 0; k < 4; ++k) {
                                                if (!util_is_double_inf_or_nan(test->unpacked[i][j][k]) &&
                           /* Ignore errors in S3TC for now */
                        if (!match) {
      printf("FAILED\n");
   printf("  Packed: %02x %02x %02x %02x\n",
         printf("  Unpacked (%u,%u): %.9g %.9g %.9g %.9g obtained\n",
         j, i,
   printf("                  %.9g %.9g %.9g %.9g expected\n",
         test->unpacked[i][j][0],
   test->unpacked[i][j][1],
   test->unpacked[i][j][2],
   fflush(stdout);
                           gallivm_destroy(gallivm);
            if (fp)
               }
         UTIL_ALIGN_STACK
   static bool
   test_format_unorm8(unsigned verbose, FILE *fp,
               {
      LLVMContextRef context;
   struct gallivm_state *gallivm;
   LLVMValueRef fetch = NULL;
   fetch_ptr_t fetch_ptr;
   alignas(16) uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   uint8_t unpacked[4];
   bool first = true;
   bool success = true;
               #if LLVM_VERSION_MAJOR == 15
         #endif
               fetch = add_fetch_rgba_test(gallivm, verbose, desc,
                                       for (l = 0; l < util_format_nr_test_cases; ++l) {
                           if (first) {
      printf("Testing %s (unorm8) ...\n",
                     /* To ensure it's 16-byte aligned */
                  for (i = 0; i < desc->block.height; ++i) {
                                                                                                                  /* Ignore errors in S3TC as we only implement a poor man approach */
                        if (!match) {
      printf("FAILED\n");
   printf("  Packed: %02x %02x %02x %02x\n",
         printf("  Unpacked (%u,%u): %02x %02x %02x %02x obtained\n",
         j, i,
   printf("                  %02x %02x %02x %02x expected\n",
                                                      gallivm_destroy(gallivm);
            if (fp)
               }
               static bool
   test_one(unsigned verbose, FILE *fp,
               {
               if (!test_format_float(verbose, fp, format_desc, use_cache)) {
   success = false;
            if (!test_format_unorm8(verbose, fp, format_desc, use_cache)) {
   success = false;
               }
         bool
   test_all(unsigned verbose, FILE *fp)
   {
      enum pipe_format format;
   bool success = true;
                     for (use_cache = 0; use_cache < 2; use_cache++) {
      for (format = 1; format < PIPE_FORMAT_COUNT; ++format) {
                        /*
                  if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
                                 /* The codegen sometimes falls back to calling the precompiled fetch
   * func, so if we don't have one of those (some compressed formats,
   * some ), we can't reliably test it.  We'll surely have a
   * precompiled fetch func for any format before we write LLVM code to
   * fetch from it.
   */
                  /* only test twice with formats which can use cache */
   if (format_desc->layout != UTIL_FORMAT_LAYOUT_S3TC && use_cache) {
                  if (!test_one(verbose, fp, format_desc, use_cache)) {
               }
               }
         bool
   test_some(unsigned verbose, FILE *fp,
         {
         }
         bool
   test_single(unsigned verbose, FILE *fp)
   {
      printf("no test_single()");
      }
