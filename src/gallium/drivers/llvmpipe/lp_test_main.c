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
   * Shared testing code.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/u_math.h"
      #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_debug.h"
   #include "lp_test.h"
         void
   dump_type(FILE *fp,
         {
      fprintf(fp, "%s%s%u%sx%u",
         type.sign ? (type.floating || type.fixed ? "" : "s") : "u",
   type.floating ? "f" : (type.fixed ? "h" : "i"),
   type.width,
      }
         double
   read_elem(struct lp_type type, const void *src, unsigned index)
   {
      double scale = lp_const_scale(type);
   double value;
   assert(index < type.length);
   if (type.floating) {
      switch (type.width) {
   case 32:
      value = *((const float *)src + index);
      case 64:
      value =  *((const double *)src + index);
      default:
      assert(0);
         } else {
      if (type.sign) {
      switch (type.width) {
   case 8:
      value = *((const int8_t *)src + index);
      case 16:
      value = *((const int16_t *)src + index);
      case 32:
      value = *((const int32_t *)src + index);
      case 64:
      value = *((const int64_t *)src + index);
      default:
      assert(0);
         } else {
      switch (type.width) {
   case 8:
      value = *((const uint8_t *)src + index);
      case 16:
      value = *((const uint16_t *)src + index);
      case 32:
      value = *((const uint32_t *)src + index);
      case 64:
      value = *((const uint64_t *)src + index);
      default:
      assert(0);
            }
      }
         void
   write_elem(struct lp_type type, void *dst, unsigned index, double value)
   {
      assert(index < type.length);
   if (!type.sign && value < 0.0)
         if (type.norm && value < -1.0)
         if (type.norm && value > 1.0)
         if (type.floating) {
      switch (type.width) {
   case 32:
      *((float *)dst + index) = (float)(value);
      case 64:
      *((double *)dst + index) = value;
      default:
            } else {
      double scale = lp_const_scale(type);
   value = round(value*scale);
   if (type.sign) {
      long long lvalue = (long long)value;
   lvalue = MIN2(lvalue, ((long long)1 << (type.width - 1)) - 1);
   lvalue = MAX2(lvalue, -((long long)1 << (type.width - 1)));
   switch (type.width) {
   case 8:
      *((int8_t *)dst + index) = (int8_t)lvalue;
      case 16:
      *((int16_t *)dst + index) = (int16_t)lvalue;
      case 32:
      *((int32_t *)dst + index) = (int32_t)lvalue;
      case 64:
      *((int64_t *)dst + index) = (int64_t)lvalue;
      default:
            } else {
      unsigned long long lvalue = (long long)value;
   lvalue = MIN2(lvalue, ((unsigned long long)1 << type.width) - 1);
   switch (type.width) {
   case 8:
      *((uint8_t *)dst + index) = (uint8_t)lvalue;
      case 16:
      *((uint16_t *)dst + index) = (uint16_t)lvalue;
      case 32:
      *((uint32_t *)dst + index) = (uint32_t)lvalue;
      case 64:
      *((uint64_t *)dst + index) = (uint64_t)lvalue;
      default:
                  }
         void
   random_elem(struct lp_type type, void *dst, unsigned index)
   {
      double value;
   assert(index < type.length);
   value = (double)rand()/(double)RAND_MAX;
   if (!type.norm) {
      if (type.floating) {
         } else {
      unsigned long long mask;
   if (type.fixed)
         else if (type.sign)
         else
         value += (double)(mask & rand());
   if (!type.fixed && !type.sign && type.width == 32) {
      /*
   * rand only returns half the possible range
   * XXX 64bit values...
   */
   if (rand() & 1)
            }
   if (type.sign)
      if (rand() & 1)
         }
         void
   read_vec(struct lp_type type, const void *src, double *dst)
   {
      unsigned i;
   for (i = 0; i < type.length; ++i)
      }
         void
   write_vec(struct lp_type type, void *dst, const double *src)
   {
      unsigned i;
   for (i = 0; i < type.length; ++i)
      }
         float
   random_float(void)
   {
         }
         void
   random_vec(struct lp_type type, void *dst)
   {
      unsigned i;
   for (i = 0; i < type.length; ++i)
      }
         bool
   compare_vec_with_eps(struct lp_type type, const void *res, const void *ref, double eps)
   {
      unsigned i;
   eps *= type.floating ? 8.0 : 2.0;
   for (i = 0; i < type.length; ++i) {
      double res_elem = read_elem(type, res, i);
   double ref_elem = read_elem(type, ref, i);
   double delta = res_elem - ref_elem;
   if (ref_elem < -1.0 || ref_elem > 1.0) {
         }
   delta = fabs(delta);
   if (delta >= eps) {
                        }
         bool
   compare_vec(struct lp_type type, const void *res, const void *ref)
   {
      double eps = lp_const_eps(type);
      }
         void
   dump_vec(FILE *fp, struct lp_type type, const void *src)
   {
      unsigned i;
   for (i = 0; i < type.length; ++i) {
      if (i)
         if (type.floating) {
      double value;
   switch (type.width) {
   case 32:
      value = *((const float *)src + i);
      case 64:
      value = *((const double *)src + i);
      default:
      assert(0);
      }
      } else {
      if (type.sign && !type.norm) {
      long long value;
   const char *format;
   switch (type.width) {
   case 8:
      value = *((const int8_t *)src + i);
   format = "%3lli";
      case 16:
      value = *((const int16_t *)src + i);
   format = "%5lli";
      case 32:
      value = *((const int32_t *)src + i);
   format = "%10lli";
      case 64:
      value = *((const int64_t *)src + i);
   format = "%20lli";
      default:
      assert(0);
   value = 0.0;
      }
      } else {
      unsigned long long value;
   const char *format;
   switch (type.width) {
   case 8:
      value = *((const uint8_t *)src + i);
   format = type.norm ? "%2x" : "%4llu";
      case 16:
      value = *((const uint16_t *)src + i);
   format = type.norm ? "%4x" : "%6llx";
      case 32:
      value = *((const uint32_t *)src + i);
   format = type.norm ? "%8x" : "%11llx";
      case 64:
      value = *((const uint64_t *)src + i);
   format = type.norm ? "%16x" : "%21llx";
      default:
      assert(0);
   value = 0.0;
      }
               }
         int
   main(int argc, char **argv)
   {
      unsigned verbose = 0;
   FILE *fp = NULL;
   unsigned long n = 1000;
   unsigned i;
   bool success;
   bool single = false;
            fpstate = util_fpstate_get();
            if (!lp_build_init())
            for (i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-v") == 0)
         else if (strcmp(argv[i], "-s") == 0)
         else if (strcmp(argv[i], "-o") == 0)
         else
            #ifdef DEBUG
      if (verbose >= 2) {
      gallivm_debug |= GALLIVM_DEBUG_IR;
         #endif
         if (fp) {
      /* Warm up the caches */
                        if (single)
         else if (n)
         else
            if (fp)
                        }
