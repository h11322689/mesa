   /*
   * Copyright (C) 2018-2019 Alyssa Rosenzweig <alyssa@rosenzweig.io>
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <inttypes.h>
   #include <math.h>
   #include "util/half_float.h"
   #include "helpers.h"
   #include "midgard.h"
   #include "midgard_ops.h"
      void
   mir_print_constant_component(FILE *fp, const midgard_constants *consts,
               {
      bool is_sint = false, is_uint = false, is_hex = false;
                     /* Add a sentinel name to prevent crashing */
   if (!opname)
            if (is_int) {
               if (!is_uint) {
      /* Bit ops are easier to follow when the constant is printed in
   * hexadecimal. Other operations starting with a 'i' are
   * considered to operate on signed integers. That might not
   * be true for all of them, but it's good enough for traces.
   */
   if (op >= midgard_alu_op_iand && op <= midgard_alu_op_ipopcnt)
         else
                  if (half)
            switch (reg_mode) {
   case midgard_reg_mode_64:
      if (is_sint) {
         } else if (is_uint) {
         } else if (is_hex) {
         } else {
               if (mod & MIDGARD_FLOAT_MOD_ABS)
                           }
         case midgard_reg_mode_32:
      if (is_sint) {
               if (half && mod == midgard_int_zero_extend)
         else if (half && mod == midgard_int_left_shift)
                           } else if (is_uint || is_hex) {
               if (half && mod == midgard_int_left_shift)
                           } else {
               if (mod & MIDGARD_FLOAT_MOD_ABS)
                           }
         case midgard_reg_mode_16:
      if (is_sint) {
               if (half && mod == midgard_int_zero_extend)
         else if (half && mod == midgard_int_left_shift)
                           } else if (is_uint || is_hex) {
               if (half && mod == midgard_int_left_shift)
                           } else {
               if (mod & MIDGARD_FLOAT_MOD_ABS)
                           }
         case midgard_reg_mode_8:
               if (mod)
                           }
      static char *outmod_names_float[4] = {"", ".clamp_0_inf", ".clamp_m1_1",
            static char *outmod_names_int[4] = {".ssat", ".usat", ".keeplo", ".keephi"};
      void
   mir_print_outmod(FILE *fp, unsigned outmod, bool is_int)
   {
      fprintf(fp, "%s",
      }
