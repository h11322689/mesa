   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         #include "lp_bld_format.h"
      LLVMTypeRef lp_build_format_cache_elem_type(struct gallivm_state *gallivm, enum cache_member member) {
      assert(member == LP_BUILD_FORMAT_CACHE_MEMBER_DATA || member == LP_BUILD_FORMAT_CACHE_MEMBER_TAGS);
   switch (member) {
   case LP_BUILD_FORMAT_CACHE_MEMBER_DATA:
         case LP_BUILD_FORMAT_CACHE_MEMBER_TAGS:
         default:
            }
      LLVMTypeRef lp_build_format_cache_member_type(struct gallivm_state *gallivm, enum cache_member member) {
      assert(member == LP_BUILD_FORMAT_CACHE_MEMBER_DATA || member == LP_BUILD_FORMAT_CACHE_MEMBER_TAGS);
   unsigned elem_count =
         member == LP_BUILD_FORMAT_CACHE_MEMBER_DATA ? LP_BUILD_FORMAT_CACHE_SIZE * 16 :
      }
      LLVMTypeRef
   lp_build_format_cache_type(struct gallivm_state *gallivm)
   {
      LLVMTypeRef elem_types[LP_BUILD_FORMAT_CACHE_MEMBER_COUNT];
            int members[] = {LP_BUILD_FORMAT_CACHE_MEMBER_DATA, LP_BUILD_FORMAT_CACHE_MEMBER_TAGS};
   for (int i = 0; i < ARRAY_SIZE(members); ++i) {
      int member = members[i];
            #if LP_BUILD_FORMAT_CACHE_DEBUG
      elem_types[LP_BUILD_FORMAT_CACHE_MEMBER_ACCESS_TOTAL] =
         elem_types[LP_BUILD_FORMAT_CACHE_MEMBER_ACCESS_MISS] =
      #endif
         s = LLVMStructTypeInContext(gallivm->context, elem_types,
               }
