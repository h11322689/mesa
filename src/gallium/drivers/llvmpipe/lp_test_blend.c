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
   * Unit tests for blend LLVM IR generation
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   *
   * Blend computation code derived from code written by
   * @author Brian Paul <brian@vmware.com>
   */
      #include "util/u_memory.h"
      #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_type.h"
   #include "gallivm/lp_bld_debug.h"
   #include "lp_bld_blend.h"
   #include "lp_test.h"
         typedef void (*blend_test_ptr_t)(const void *src, const void *src1,
               void
   write_tsv_header(FILE *fp)
   {
      fprintf(fp,
         "result\t"
   "cycles_per_channel\t"
   "type\t"
   "sep_func\t"
   "sep_src_factor\t"
   "sep_dst_factor\t"
   "rgb_func\t"
   "rgb_src_factor\t"
   "rgb_dst_factor\t"
   "alpha_func\t"
               }
         static void
   write_tsv_row(FILE *fp,
               const struct pipe_blend_state *blend,
   struct lp_type type,
   {
                        fprintf(fp, "%s%u%sx%u\t",
         type.floating ? "f" : (type.fixed ? "h" : (type.sign ? "s" : "u")),
   type.width,
            fprintf(fp,
         "%s\t%s\t%s\t",
   blend->rt[0].rgb_func != blend->rt[0].alpha_func ? "true" : "false",
            fprintf(fp,
         "%s\t%s\t%s\t%s\t%s\t%s\n",
   util_str_blend_func(blend->rt[0].rgb_func, true),
   util_str_blend_factor(blend->rt[0].rgb_src_factor, true),
   util_str_blend_factor(blend->rt[0].rgb_dst_factor, true),
   util_str_blend_func(blend->rt[0].alpha_func, true),
               }
         static void
   dump_blend_type(FILE *fp,
               {
      fprintf(fp, " type=%s%u%sx%u",
         type.floating ? "f" : (type.fixed ? "h" : (type.sign ? "s" : "u")),
   type.width,
            fprintf(fp,
         " %s=%s %s=%s %s=%s %s=%s %s=%s %s=%s",
   "rgb_func",         util_str_blend_func(blend->rt[0].rgb_func, true),
   "rgb_src_factor",   util_str_blend_factor(blend->rt[0].rgb_src_factor, true),
   "rgb_dst_factor",   util_str_blend_factor(blend->rt[0].rgb_dst_factor, true),
   "alpha_func",       util_str_blend_func(blend->rt[0].alpha_func, true),
            fprintf(fp, " ...\n");
      }
         static LLVMValueRef
   add_blend_test(struct gallivm_state *gallivm,
               {
      LLVMModuleRef module = gallivm->module;
   LLVMContextRef context = gallivm->context;
   LLVMTypeRef vec_type;
   LLVMTypeRef args[5];
   LLVMValueRef func;
   LLVMValueRef src_ptr;
   LLVMValueRef src1_ptr;
   LLVMValueRef dst_ptr;
   LLVMValueRef const_ptr;
   LLVMValueRef res_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   const enum pipe_format format = PIPE_FORMAT_R8G8B8A8_UNORM;
   const unsigned rt = 0;
   const unsigned char swizzle[4] = { 0, 1, 2, 3 };
   LLVMValueRef src;
   LLVMValueRef src1;
   LLVMValueRef dst;
   LLVMValueRef con;
                     args[4] = args[3] = args[2] = args[1] = args[0] = LLVMPointerType(vec_type, 0);
   func = LLVMAddFunction(module, "test", LLVMFunctionType(LLVMVoidTypeInContext(context), args, 5, 0));
   LLVMSetFunctionCallConv(func, LLVMCCallConv);
   src_ptr = LLVMGetParam(func, 0);
   src1_ptr = LLVMGetParam(func, 1);
   dst_ptr = LLVMGetParam(func, 2);
   const_ptr = LLVMGetParam(func, 3);
            block = LLVMAppendBasicBlockInContext(context, func, "entry");
   builder = gallivm->builder;
            src = LLVMBuildLoad2(builder, vec_type, src_ptr, "src");
   src1 = LLVMBuildLoad2(builder, vec_type, src1_ptr, "src1");
   dst = LLVMBuildLoad2(builder, vec_type, dst_ptr, "dst");
            res = lp_build_blend_aos(gallivm, blend, format, type, rt, src, NULL,
                                                   }
         static void
   compute_blend_ref_term(unsigned rgb_factor,
                        unsigned alpha_factor,
   const double *factor,
   const double *src,
      {
               switch (rgb_factor) {
   case PIPE_BLENDFACTOR_ONE:
      term[0] = factor[0]; /* R */
   term[1] = factor[1]; /* G */
   term[2] = factor[2]; /* B */
      case PIPE_BLENDFACTOR_SRC_COLOR:
      term[0] = factor[0] * src[0]; /* R */
   term[1] = factor[1] * src[1]; /* G */
   term[2] = factor[2] * src[2]; /* B */
      case PIPE_BLENDFACTOR_SRC_ALPHA:
      term[0] = factor[0] * src[3]; /* R */
   term[1] = factor[1] * src[3]; /* G */
   term[2] = factor[2] * src[3]; /* B */
      case PIPE_BLENDFACTOR_DST_COLOR:
      term[0] = factor[0] * dst[0]; /* R */
   term[1] = factor[1] * dst[1]; /* G */
   term[2] = factor[2] * dst[2]; /* B */
      case PIPE_BLENDFACTOR_DST_ALPHA:
      term[0] = factor[0] * dst[3]; /* R */
   term[1] = factor[1] * dst[3]; /* G */
   term[2] = factor[2] * dst[3]; /* B */
      case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      temp = MIN2(src[3], 1.0f - dst[3]);
   term[0] = factor[0] * temp; /* R */
   term[1] = factor[1] * temp; /* G */
   term[2] = factor[2] * temp; /* B */
      case PIPE_BLENDFACTOR_CONST_COLOR:
      term[0] = factor[0] * con[0]; /* R */
   term[1] = factor[1] * con[1]; /* G */
   term[2] = factor[2] * con[2]; /* B */
      case PIPE_BLENDFACTOR_CONST_ALPHA:
      term[0] = factor[0] * con[3]; /* R */
   term[1] = factor[1] * con[3]; /* G */
   term[2] = factor[2] * con[3]; /* B */
      case PIPE_BLENDFACTOR_SRC1_COLOR:
      term[0] = factor[0] * src1[0]; /* R */
   term[1] = factor[1] * src1[1]; /* G */
   term[2] = factor[2] * src1[2]; /* B */
      case PIPE_BLENDFACTOR_SRC1_ALPHA:
      term[0] = factor[0] * src1[3]; /* R */
   term[1] = factor[1] * src1[3]; /* G */
   term[2] = factor[2] * src1[3]; /* B */
      case PIPE_BLENDFACTOR_ZERO:
      term[0] = 0.0f; /* R */
   term[1] = 0.0f; /* G */
   term[2] = 0.0f; /* B */
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      term[0] = factor[0] * (1.0f - src[0]); /* R */
   term[1] = factor[1] * (1.0f - src[1]); /* G */
   term[2] = factor[2] * (1.0f - src[2]); /* B */
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      term[0] = factor[0] * (1.0f - src[3]); /* R */
   term[1] = factor[1] * (1.0f - src[3]); /* G */
   term[2] = factor[2] * (1.0f - src[3]); /* B */
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      term[0] = factor[0] * (1.0f - dst[3]); /* R */
   term[1] = factor[1] * (1.0f - dst[3]); /* G */
   term[2] = factor[2] * (1.0f - dst[3]); /* B */
      case PIPE_BLENDFACTOR_INV_DST_COLOR:
      term[0] = factor[0] * (1.0f - dst[0]); /* R */
   term[1] = factor[1] * (1.0f - dst[1]); /* G */
   term[2] = factor[2] * (1.0f - dst[2]); /* B */
      case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      term[0] = factor[0] * (1.0f - con[0]); /* R */
   term[1] = factor[1] * (1.0f - con[1]); /* G */
   term[2] = factor[2] * (1.0f - con[2]); /* B */
      case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      term[0] = factor[0] * (1.0f - con[3]); /* R */
   term[1] = factor[1] * (1.0f - con[3]); /* G */
   term[2] = factor[2] * (1.0f - con[3]); /* B */
      case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      term[0] = factor[0] * (1.0f - src1[0]); /* R */
   term[1] = factor[1] * (1.0f - src1[1]); /* G */
   term[2] = factor[2] * (1.0f - src1[2]); /* B */
      case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      term[0] = factor[0] * (1.0f - src1[3]); /* R */
   term[1] = factor[1] * (1.0f - src1[3]); /* G */
   term[2] = factor[2] * (1.0f - src1[3]); /* B */
      default:
                  /*
   * Compute src/first term A
   */
   switch (alpha_factor) {
   case PIPE_BLENDFACTOR_ONE:
      term[3] = factor[3]; /* A */
      case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      term[3] = factor[3] * src[3]; /* A */
      case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_DST_ALPHA:
      term[3] = factor[3] * dst[3]; /* A */
      case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      term[3] = src[3]; /* A */
      case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      term[3] = factor[3] * con[3]; /* A */
      case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      term[3] = factor[3] * src1[3]; /* A */
      case PIPE_BLENDFACTOR_ZERO:
      term[3] = 0.0f; /* A */
      case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      term[3] = factor[3] * (1.0f - src[3]); /* A */
      case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      term[3] = factor[3] * (1.0f - dst[3]); /* A */
      case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      term[3] = factor[3] * (1.0f - con[3]);
      case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      term[3] = factor[3] * (1.0f - src1[3]); /* A */
      default:
            }
         static void
   compute_blend_ref(const struct pipe_blend_state *blend,
                     const double *src,
   const double *src1,
   {
      double src_term[4];
            compute_blend_ref_term(blend->rt[0].rgb_src_factor, blend->rt[0].alpha_src_factor,
         compute_blend_ref_term(blend->rt[0].rgb_dst_factor, blend->rt[0].alpha_dst_factor,
            /*
   * Combine RGB terms
   */
   switch (blend->rt[0].rgb_func) {
   case PIPE_BLEND_ADD:
      res[0] = src_term[0] + dst_term[0]; /* R */
   res[1] = src_term[1] + dst_term[1]; /* G */
   res[2] = src_term[2] + dst_term[2]; /* B */
      case PIPE_BLEND_SUBTRACT:
      res[0] = src_term[0] - dst_term[0]; /* R */
   res[1] = src_term[1] - dst_term[1]; /* G */
   res[2] = src_term[2] - dst_term[2]; /* B */
      case PIPE_BLEND_REVERSE_SUBTRACT:
      res[0] = dst_term[0] - src_term[0]; /* R */
   res[1] = dst_term[1] - src_term[1]; /* G */
   res[2] = dst_term[2] - src_term[2]; /* B */
      case PIPE_BLEND_MIN:
      res[0] = MIN2(src_term[0], dst_term[0]); /* R */
   res[1] = MIN2(src_term[1], dst_term[1]); /* G */
   res[2] = MIN2(src_term[2], dst_term[2]); /* B */
      case PIPE_BLEND_MAX:
      res[0] = MAX2(src_term[0], dst_term[0]); /* R */
   res[1] = MAX2(src_term[1], dst_term[1]); /* G */
   res[2] = MAX2(src_term[2], dst_term[2]); /* B */
      default:
                  /*
   * Combine A terms
   */
   switch (blend->rt[0].alpha_func) {
   case PIPE_BLEND_ADD:
      res[3] = src_term[3] + dst_term[3]; /* A */
      case PIPE_BLEND_SUBTRACT:
      res[3] = src_term[3] - dst_term[3]; /* A */
      case PIPE_BLEND_REVERSE_SUBTRACT:
      res[3] = dst_term[3] - src_term[3]; /* A */
      case PIPE_BLEND_MIN:
      res[3] = MIN2(src_term[3], dst_term[3]); /* A */
      case PIPE_BLEND_MAX:
      res[3] = MAX2(src_term[3], dst_term[3]); /* A */
      default:
            }
         UTIL_ALIGN_STACK
   static bool
   test_one(unsigned verbose,
            FILE *fp,
      {
      LLVMContextRef context;
   struct gallivm_state *gallivm;
   LLVMValueRef func = NULL;
   blend_test_ptr_t blend_test_ptr;
   bool success;
   const unsigned n = LP_TEST_NUM_SAMPLES;
   int64_t cycles[LP_TEST_NUM_SAMPLES];
   double cycles_avg = 0.0;
   unsigned i, j;
            if (verbose >= 1)
               #if LLVM_VERSION_MAJOR == 15
         #endif
                                                            {
      uint8_t *src, *src1, *dst, *con, *res, *ref;
   src = align_malloc(stride, stride);
   src1 = align_malloc(stride, stride);
   dst = align_malloc(stride, stride);
   con = align_malloc(stride, stride);
   res = align_malloc(stride, stride);
            for (i = 0; i < n && success; ++i) {
                     random_vec(type, src);
   random_vec(type, src1);
                  {
      double fsrc[LP_MAX_VECTOR_LENGTH];
   double fsrc1[LP_MAX_VECTOR_LENGTH];
   double fdst[LP_MAX_VECTOR_LENGTH];
                  read_vec(type, src, fsrc);
   read_vec(type, src1, fsrc1);
                                             start_counter = rdtsc();
                                             if (verbose < 1)
                  fprintf(stderr, "  Src: ");
                  fprintf(stderr, "  Src1: ");
                  fprintf(stderr, "  Dst: ");
                  fprintf(stderr, "  Con: ");
                  fprintf(stderr, "  Res: ");
                  fprintf(stderr, "  Ref: ");
   dump_vec(stderr, type, ref);
         }
   align_free(src);
   align_free(src1);
   align_free(dst);
   align_free(con);
   align_free(res);
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
         const unsigned
   blend_factors[] = {
      PIPE_BLENDFACTOR_ZERO,
   PIPE_BLENDFACTOR_ONE,
   PIPE_BLENDFACTOR_SRC_COLOR,
   PIPE_BLENDFACTOR_SRC_ALPHA,
   PIPE_BLENDFACTOR_DST_COLOR,
   PIPE_BLENDFACTOR_DST_ALPHA,
   PIPE_BLENDFACTOR_CONST_COLOR,
   PIPE_BLENDFACTOR_CONST_ALPHA,
   PIPE_BLENDFACTOR_SRC1_COLOR,
   PIPE_BLENDFACTOR_SRC1_ALPHA,
   PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE,
   PIPE_BLENDFACTOR_INV_SRC_COLOR,
   PIPE_BLENDFACTOR_INV_SRC_ALPHA,
   PIPE_BLENDFACTOR_INV_DST_COLOR,
   PIPE_BLENDFACTOR_INV_DST_ALPHA,
   PIPE_BLENDFACTOR_INV_CONST_COLOR,
   PIPE_BLENDFACTOR_INV_CONST_ALPHA,
   PIPE_BLENDFACTOR_INV_SRC1_COLOR,
      };
         const unsigned
   blend_funcs[] = {
      PIPE_BLEND_ADD,
   PIPE_BLEND_SUBTRACT,
   PIPE_BLEND_REVERSE_SUBTRACT,
   PIPE_BLEND_MIN,
      };
         const struct lp_type blend_types[] = {
      /* float, fixed,  sign,  norm, width, len */
   {   true, false,  true, false,    32,   4 }, /* f32 x 4 */
      };
         const unsigned num_funcs = ARRAY_SIZE(blend_funcs);
   const unsigned num_factors = ARRAY_SIZE(blend_factors);
   const unsigned num_types = ARRAY_SIZE(blend_types);
         bool
   test_all(unsigned verbose, FILE *fp)
   {
      const unsigned *rgb_func;
   const unsigned *rgb_src_factor;
   const unsigned *rgb_dst_factor;
   const unsigned *alpha_func;
   const unsigned *alpha_src_factor;
   const unsigned *alpha_dst_factor;
   struct pipe_blend_state blend;
   const struct lp_type *type;
            for (rgb_func = blend_funcs; rgb_func < &blend_funcs[num_funcs]; ++rgb_func) {
      for (alpha_func = blend_funcs; alpha_func < &blend_funcs[num_funcs]; ++alpha_func) {
      for (rgb_src_factor = blend_factors; rgb_src_factor < &blend_factors[num_factors]; ++rgb_src_factor) {
      for (rgb_dst_factor = blend_factors; rgb_dst_factor <= rgb_src_factor; ++rgb_dst_factor) {
                                                         memset(&blend, 0, sizeof blend);
   blend.rt[0].blend_enable      = 1;
   blend.rt[0].rgb_func          = *rgb_func;
   blend.rt[0].rgb_src_factor    = *rgb_src_factor;
   blend.rt[0].rgb_dst_factor    = *rgb_dst_factor;
                                                                        }
         bool
   test_some(unsigned verbose, FILE *fp,
         {
      const unsigned *rgb_func;
   const unsigned *rgb_src_factor;
   const unsigned *rgb_dst_factor;
   const unsigned *alpha_func;
   const unsigned *alpha_src_factor;
   const unsigned *alpha_dst_factor;
   struct pipe_blend_state blend;
   const struct lp_type *type;
            for (unsigned long i = 0; i < n; ++i) {
      rgb_func = &blend_funcs[rand() % num_funcs];
   alpha_func = &blend_funcs[rand() % num_funcs];
   rgb_src_factor = &blend_factors[rand() % num_factors];
            do {
                  do {
                           memset(&blend, 0, sizeof blend);
   blend.rt[0].blend_enable      = 1;
   blend.rt[0].rgb_func          = *rgb_func;
   blend.rt[0].rgb_src_factor    = *rgb_src_factor;
   blend.rt[0].rgb_dst_factor    = *rgb_dst_factor;
   blend.rt[0].alpha_func        = *alpha_func;
   blend.rt[0].alpha_src_factor  = *alpha_src_factor;
   blend.rt[0].alpha_dst_factor  = *alpha_dst_factor;
            if (!test_one(verbose, fp, &blend, *type))
                  }
         bool
   test_single(unsigned verbose, FILE *fp)
   {
      printf("no test_single()");
      }
