   /*
   * Copyright © 2019 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #undef NDEBUG
      #include <stdio.h>
   #include <stdint.h>
   #include <stdbool.h>
   #include "intel_decoder.h"
      static bool quiet = false;
      struct test_address {
         };
      __attribute__((unused)) static uint64_t
   _test_combine_address(void *data, void *location,
         {
         }
      #define __gen_user_data void
   #define __gen_combine_address _test_combine_address
   #define __gen_address_type struct test_address
      #include "gentest_pack.h"
      static void
   test_struct(struct intel_spec *spec) {
      /* Fill struct fields and <group> tag */
   struct GFX9_TEST_STRUCT test1 = {
      .number1 = 5,
               for (int i = 0; i < 4; i++) {
                  /* Pack struct into a dw array */
   uint32_t dw[GFX9_TEST_STRUCT_length];
            /* Now decode the packed struct, and make sure it matches the original */
   struct intel_group *group;
                     if (!quiet) {
      printf("\nTEST_STRUCT:\n");
               struct intel_field_iterator iter;
            while (intel_field_iterator_next(&iter)) {
      int idx;
   if (strcmp(iter.name, "number1") == 0) {
      uint16_t number = iter.raw_value;
      } else if (strcmp(iter.name, "number2") == 0) {
      uint16_t number = iter.raw_value;
      } else if (sscanf(iter.name, "byte[%d]", &idx) == 1) {
      uint8_t number = iter.raw_value;
            }
      static void
   test_two_levels(struct intel_spec *spec) {
               for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 8; j++) {
                     uint32_t dw[GFX9_STRUCT_TWO_LEVELS_length];
            struct intel_group *group;
                     if (!quiet) {
      printf("\nSTRUCT_TWO_LEVELS\n");
               struct intel_field_iterator iter;
            while (intel_field_iterator_next(&iter)) {
               assert(sscanf(iter.name, "byte[%d][%d]", &i, &j) == 2);
   uint8_t number = iter.raw_value;
         }
      int main(int argc, char **argv)
   {
               if (argc > 1 && strcmp(argv[1], "-quiet") == 0)
            test_struct(spec);
                        }
