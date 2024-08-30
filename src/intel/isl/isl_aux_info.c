   /*
   * Copyright 2019 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "isl/isl.h"
      #ifdef IN_UNIT_TEST
   /* STATIC_ASSERT is a do { ... } while(0) statement */
   UNUSED static void static_assert_func(void) {
      STATIC_ASSERT(ISL_AUX_OP_ASSERT == ((enum isl_aux_op) 0));
      }
      #undef unreachable
   #define unreachable(str) return 0
      #undef assert
   #define assert(cond) do { \
      if (!(cond)) { \
            } while (0)
   #endif
      /* How writes with an isl_aux_usage behave. */
   enum write_behavior {
      /* Writes only touch the main surface. */
            /* Writes using the 3D engine are compressed. */
            /* Writes using the 3D engine are either compressed or substituted with
   * fast-cleared blocks.
   */
            /* Writes implicitly fully resolve the compression block and write the data
   * uncompressed into the main surface. The resolved aux blocks are
   * ambiguated and left in the pass-through state.
   */
      };
      /* A set of features supported by an isl_aux_usage. */
   struct aux_usage_info {
         /* How writes affect the surface(s) in use. */
            /* Aux supports "real" compression beyond just fast-clears. */
            /* SW can perform ISL_AUX_OP_FAST_CLEAR. */
            /* SW can perform ISL_AUX_OP_PARTIAL_RESOLVE. */
            /* Performing ISL_AUX_OP_FULL_RESOLVE includes ISL_AUX_OP_AMBIGUATE. */
      };
      #define AUX(wb, c, fc, pr, fra, type)                   \
         #define Y true
   #define x false
   static const struct aux_usage_info info[] = {
   /*         write_behavior c fc pr fra */
      AUX(         COMPRESS, Y, Y, x, x, HIZ)
   AUX(         COMPRESS, Y, Y, x, x, HIZ_CCS)
   AUX(         COMPRESS, Y, Y, x, x, HIZ_CCS_WT)
   AUX(         COMPRESS, Y, Y, Y, x, MCS)
   AUX(         COMPRESS, Y, Y, Y, x, MCS_CCS)
   AUX(         COMPRESS, Y, Y, Y, Y, CCS_E)
   AUX(   COMPRESS_CLEAR, Y, Y, Y, Y, FCV_CCS_E)
   AUX(RESOLVE_AMBIGUATE, x, Y, x, Y, CCS_D)
   AUX(RESOLVE_AMBIGUATE, Y, x, x, Y, MC)
      };
   #undef x
   #undef Y
   #undef AUX
      ASSERTED static bool
   aux_state_possible(enum isl_aux_state state,
         {
      switch (state) {
   case ISL_AUX_STATE_CLEAR:
   case ISL_AUX_STATE_PARTIAL_CLEAR:
         case ISL_AUX_STATE_COMPRESSED_CLEAR:
         case ISL_AUX_STATE_COMPRESSED_NO_CLEAR:
         case ISL_AUX_STATE_RESOLVED:
   case ISL_AUX_STATE_PASS_THROUGH:
   case ISL_AUX_STATE_AUX_INVALID:
      #ifdef IN_UNIT_TEST
      case ISL_AUX_STATE_ASSERT:
      #endif
                  }
      enum isl_aux_op
   isl_aux_prepare_access(enum isl_aux_state initial_state,
               {
      if (usage != ISL_AUX_USAGE_NONE) {
      UNUSED const enum isl_aux_usage state_superset_usage =
            }
            switch (initial_state) {
   case ISL_AUX_STATE_COMPRESSED_CLEAR:
      if (!info[usage].compressed)
            case ISL_AUX_STATE_CLEAR:
   case ISL_AUX_STATE_PARTIAL_CLEAR:
      return fast_clear_supported ?
                  case ISL_AUX_STATE_COMPRESSED_NO_CLEAR:
      return info[usage].compressed ?
      case ISL_AUX_STATE_RESOLVED:
   case ISL_AUX_STATE_PASS_THROUGH:
         case ISL_AUX_STATE_AUX_INVALID:
      return info[usage].write_behavior == WRITES_ONLY_TOUCH_MAIN ?
   #ifdef IN_UNIT_TEST
      case ISL_AUX_STATE_ASSERT:
      #endif
                  }
      enum isl_aux_state
   isl_aux_state_transition_aux_op(enum isl_aux_state initial_state,
               {
      assert(aux_state_possible(initial_state, usage));
            switch (op) {
   case ISL_AUX_OP_NONE:
         case ISL_AUX_OP_FAST_CLEAR:
      assert(info[usage].fast_clear);
      case ISL_AUX_OP_PARTIAL_RESOLVE:
      assert(isl_aux_state_has_valid_aux(initial_state));
   assert(info[usage].partial_resolve);
   return initial_state == ISL_AUX_STATE_CLEAR ||
         initial_state == ISL_AUX_STATE_PARTIAL_CLEAR ||
      case ISL_AUX_OP_FULL_RESOLVE:
      assert(isl_aux_state_has_valid_aux(initial_state));
   return info[usage].full_resolves_ambiguate ||
            case ISL_AUX_OP_AMBIGUATE:
      #if IN_UNIT_TEST
      case ISL_AUX_OP_ASSERT:
      #endif
                  }
      enum isl_aux_state
   isl_aux_state_transition_write(enum isl_aux_state initial_state,
               {
      if (info[usage].write_behavior == WRITES_ONLY_TOUCH_MAIN) {
               return initial_state == ISL_AUX_STATE_PASS_THROUGH ?
               assert(isl_aux_state_has_valid_aux(initial_state));
   assert(aux_state_possible(initial_state, usage));
   assert(info[usage].write_behavior == WRITES_COMPRESS ||
                  if (full_surface) {
      return info[usage].write_behavior == WRITES_COMPRESS ?
                           switch (initial_state) {
   case ISL_AUX_STATE_CLEAR:
   case ISL_AUX_STATE_PARTIAL_CLEAR:
      return info[usage].write_behavior == WRITES_RESOLVE_AMBIGUATE ?
      case ISL_AUX_STATE_RESOLVED:
   case ISL_AUX_STATE_PASS_THROUGH:
   case ISL_AUX_STATE_COMPRESSED_NO_CLEAR:
      return info[usage].write_behavior == WRITES_COMPRESS ?
                  case ISL_AUX_STATE_COMPRESSED_CLEAR:
   case ISL_AUX_STATE_AUX_INVALID:
      #ifdef IN_UNIT_TEST
      case ISL_AUX_STATE_ASSERT:
      #endif
                  }
      bool
   isl_aux_usage_has_fast_clears(enum isl_aux_usage usage)
   {
         }
      bool
   isl_aux_usage_has_compression(enum isl_aux_usage usage)
   {
         }
