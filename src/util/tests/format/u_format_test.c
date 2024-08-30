   /**************************************************************************
   *
   * Copyright 2009-2010 VMware, Inc.
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
      #include "util/half_float.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "util/format/u_format_tests.h"
   #include "util/format/u_format_s3tc.h"
         static bool
   compare_float(float x, float y)
   {
               if (error < 0.0f)
            if (error > FLT_EPSILON) {
                     }
         static void
   print_packed(const struct util_format_description *format_desc,
               const char *prefix,
   {
      unsigned i;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.bits/8; ++i) {
      printf("%s%02x", sep, packed[i]);
      }
   printf("%s", suffix);
      }
         static void
   print_unpacked_rgba_doubl(const struct util_format_description *format_desc,
                     {
      unsigned i, j;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      printf("%s{%f, %f, %f, %f}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
      }
      }
   printf("%s", suffix);
      }
         static void
   print_unpacked_rgba_float(const struct util_format_description *format_desc,
                     {
      unsigned i, j;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      printf("%s{%f, %f, %f, %f}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
      }
      }
   printf("%s", suffix);
      }
         static void
   print_unpacked_rgba_8unorm(const struct util_format_description *format_desc,
                     {
      unsigned i, j;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      printf("%s{0x%02x, 0x%02x, 0x%02x, 0x%02x}", sep, unpacked[i][j][0], unpacked[i][j][1], unpacked[i][j][2], unpacked[i][j][3]);
         }
   printf("%s", suffix);
      }
         static void
   print_unpacked_z_float(const struct util_format_description *format_desc,
                     {
      unsigned i, j;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      printf("%s%f", sep, unpacked[i][j]);
      }
      }
   printf("%s", suffix);
      }
         static void
   print_unpacked_z_32unorm(const struct util_format_description *format_desc,
                     {
      unsigned i, j;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      printf("%s0x%08x", sep, unpacked[i][j]);
         }
   printf("%s", suffix);
      }
         static void
   print_unpacked_s_8uint(const struct util_format_description *format_desc,
                     {
      unsigned i, j;
            printf("%s", prefix);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      printf("%s0x%02x", sep, unpacked[i][j]);
         }
   printf("%s", suffix);
      }
         static bool
   test_format_fetch_rgba(const struct util_format_description *format_desc,
         {
      util_format_fetch_rgba_func_ptr fetch_rgba =
         float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
            success = true;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      fetch_rgba(unpacked[i][j], test->packed, j, i);
   for (k = 0; k < 4; ++k) {
      if (!compare_float(test->unpacked[i][j][k], unpacked[i][j][k])) {
                           /* Ignore S3TC errors */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
                  if (!success) {
      print_unpacked_rgba_float(format_desc, "FAILED: ", unpacked, " obtained\n");
                  }
         static bool
   test_format_unpack_rgba(const struct util_format_description *format_desc,
         {
      float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
            util_format_unpack_rgba_rect(format_desc->format, &unpacked[0][0][0], sizeof unpacked[0],
                  success = true;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      for (k = 0; k < 4; ++k) {
      if (!compare_float(test->unpacked[i][j][k], unpacked[i][j][k])) {
                           /* Ignore S3TC errors */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
                  if (!success) {
      print_unpacked_rgba_float(format_desc, "FAILED: ", unpacked, " obtained\n");
                  }
         static bool
   test_format_pack_rgba_float(const struct util_format_description *format_desc,
         {
      const struct util_format_pack_description *pack =
         float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j, k;
            if (test->format == PIPE_FORMAT_DXT1_RGBA) {
      /*
   * Skip S3TC as packed representation is not canonical.
   *
   * TODO: Do a round trip conversion.
   */
               memset(packed, 0, sizeof packed);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      for (k = 0; k < 4; ++k) {
                        pack->pack_rgba_float(packed, 0,
                  success = true;
   for (i = 0; i < format_desc->block.bits/8; ++i) {
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
               /* Ignore NaN */
   if (util_is_double_nan(test->unpacked[0][0][0]))
            /* Ignore S3TC errors */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
                  if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
                  }
         static bool
   convert_float_to_8unorm(uint8_t *dst, const double *src)
   {
      unsigned i;
            for (i = 0; i < UTIL_FORMAT_MAX_UNPACKED_HEIGHT*UTIL_FORMAT_MAX_UNPACKED_WIDTH*4; ++i) {
      if (src[i] < 0.0) {
      accurate = false;
      }
   else if (src[i] > 1.0) {
      accurate = false;
      }
   else {
                        }
         static bool
   test_format_unpack_rgba_8unorm(const struct util_format_description *format_desc,
         {
      uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   uint8_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4] = { { { 0 } } };
   unsigned i, j, k;
            if (util_format_is_pure_integer(format_desc->format))
            util_format_unpack_rgba_8unorm_rect(format_desc->format, &unpacked[0][0][0], sizeof unpacked[0],
                           success = true;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      for (k = 0; k < 4; ++k) {
      if (expected[i][j][k] != unpacked[i][j][k]) {
                           /* Ignore NaN */
   if (util_is_double_nan(test->unpacked[0][0][0]))
            if (!success) {
      print_unpacked_rgba_8unorm(format_desc, "FAILED: ", unpacked, " obtained\n");
                  }
         static bool
   test_format_pack_rgba_8unorm(const struct util_format_description *format_desc,
         {
      const struct util_format_pack_description *pack =
         uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH][4];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i;
            if (test->format == PIPE_FORMAT_DXT1_RGBA) {
      /*
   * Skip S3TC as packed representation is not canonical.
   *
   * TODO: Do a round trip conversion.
   */
               if (!convert_float_to_8unorm(&unpacked[0][0][0], &test->unpacked[0][0][0])) {
      /*
   * Skip test cases which cannot be represented by four unorm bytes.
   */
                        pack->pack_rgba_8unorm(packed, 0,
                  success = true;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         /* Ignore NaN */
   if (util_is_double_nan(test->unpacked[0][0][0]))
            /* Ignore failure cases due to unorm8 format */
   if (test->unpacked[0][0][0] > 1.0f || test->unpacked[0][0][0] < 0.0f)
            /* Multiple of 255 */
   if ((test->unpacked[0][0][0] * 255.0) != (int)(test->unpacked[0][0][0] * 255.0))
            /* Ignore S3TC errors */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
                  if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
                  }
         static bool
   test_format_unpack_z_float(const struct util_format_description *format_desc,
         {
      const struct util_format_unpack_description *unpack =
         float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   unsigned i, j;
            unpack->unpack_z_float(&unpacked[0][0], sizeof unpacked[0],
                  success = true;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      if (!compare_float(test->unpacked[i][j][0], unpacked[i][j])) {
                        if (!success) {
      print_unpacked_z_float(format_desc, "FAILED: ", unpacked, " obtained\n");
                  }
         static bool
   test_format_pack_z_float(const struct util_format_description *format_desc,
         {
      const struct util_format_pack_description *pack =
         float unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j;
            memset(packed, 0, sizeof packed);
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      unpacked[i][j] = (float) test->unpacked[i][j][0];
   if (test->unpacked[i][j][1]) {
                        pack->pack_z_float(packed, 0,
                  success = true;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
                  }
         static bool
   test_format_unpack_z_32unorm(const struct util_format_description *format_desc,
         {
      const struct util_format_unpack_description *unpack =
         uint32_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   uint32_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   unsigned i, j;
            unpack->unpack_z_32unorm(&unpacked[0][0], sizeof unpacked[0],
                  for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
                     success = true;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      if (expected[i][j] != unpacked[i][j]) {
                        if (!success) {
      print_unpacked_z_32unorm(format_desc, "FAILED: ", unpacked, " obtained\n");
                  }
         static bool
   test_format_pack_z_32unorm(const struct util_format_description *format_desc,
         {
      const struct util_format_pack_description *pack =
         uint32_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j;
            for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      unpacked[i][j] = test->unpacked[i][j][0] * 0xffffffff;
   if (test->unpacked[i][j][1]) {
                                 pack->pack_z_32unorm(packed, 0,
                  success = true;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
                  }
         static bool
   test_format_unpack_s_8uint(const struct util_format_description *format_desc,
         {
      const struct util_format_unpack_description *unpack =
         uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   uint8_t expected[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH] = { { 0 } };
   unsigned i, j;
            unpack->unpack_s_8uint(&unpacked[0][0], sizeof unpacked[0],
                  for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
                     success = true;
   for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      if (expected[i][j] != unpacked[i][j]) {
                        if (!success) {
      print_unpacked_s_8uint(format_desc, "FAILED: ", unpacked, " obtained\n");
                  }
         static bool
   test_format_pack_s_8uint(const struct util_format_description *format_desc,
         {
      const struct util_format_pack_description *pack =
         uint8_t unpacked[UTIL_FORMAT_MAX_UNPACKED_HEIGHT][UTIL_FORMAT_MAX_UNPACKED_WIDTH];
   uint8_t packed[UTIL_FORMAT_MAX_PACKED_BYTES];
   unsigned i, j;
            for (i = 0; i < format_desc->block.height; ++i) {
      for (j = 0; j < format_desc->block.width; ++j) {
      unpacked[i][j] = test->unpacked[i][j][1];
   if (test->unpacked[i][j][0]) {
                                 pack->pack_s_8uint(packed, 0,
                  success = true;
   for (i = 0; i < format_desc->block.bits/8; ++i)
      if ((test->packed[i] & test->mask[i]) != (packed[i] & test->mask[i]))
         if (!success) {
      print_packed(format_desc, "FAILED: ", packed, " obtained\n");
                  }
         /* Touch-test that the unorm/snorm flags are set up right by codegen. */
   static bool
   test_format_norm_flags(const struct util_format_description *format_desc)
   {
            #define FORMAT_CASE(format, unorm, snorm) \
      case format: \
      success = (format_desc->is_unorm == unorm && \
               switch (format_desc->format) {
      FORMAT_CASE(PIPE_FORMAT_R8G8B8A8_UNORM, true, false);
   FORMAT_CASE(PIPE_FORMAT_R8G8B8A8_SRGB, true, false);
   FORMAT_CASE(PIPE_FORMAT_R8G8B8A8_SNORM, false, true);
   FORMAT_CASE(PIPE_FORMAT_R32_FLOAT, false, false);
   FORMAT_CASE(PIPE_FORMAT_X8Z24_UNORM, true, false);
   FORMAT_CASE(PIPE_FORMAT_S8X24_UINT, false, false);
   FORMAT_CASE(PIPE_FORMAT_DXT1_RGB, true, false);
   FORMAT_CASE(PIPE_FORMAT_ETC2_RGB8, true, false);
   FORMAT_CASE(PIPE_FORMAT_ETC2_R11_SNORM, false, true);
   FORMAT_CASE(PIPE_FORMAT_ASTC_4x4, true, false);
   FORMAT_CASE(PIPE_FORMAT_BPTC_RGBA_UNORM, true, false);
      default:
      success = !(format_desc->is_unorm && format_desc->is_snorm);
         #undef FORMAT_CASE
         if (!success) {
      printf("FAILED: %s (unorm %s, snorm %s)\n",
         format_desc->short_name,
                  }
      typedef bool
   (*test_func_t)(const struct util_format_description *format_desc,
               static bool
   test_one_func(const struct util_format_description *format_desc,
               {
      unsigned i;
            printf("Testing util_format_%s_%s ...\n",
                  for (i = 0; i < util_format_nr_test_cases; ++i) {
               if (test->format == format_desc->format) {
      if (!func(format_desc, &util_format_test_cases[i])) {
   success = false;
                     }
      static bool
   test_format_metadata(const struct util_format_description *format_desc,
               {
               printf("Testing util_format_%s_%s ...\n", format_desc->short_name, suffix);
            if (!func(format_desc)) {
                     }
      static bool
   test_all(void)
   {
      enum pipe_format format;
            for (format = 1; format < PIPE_FORMAT_COUNT; ++format) {
               format_desc = util_format_description(format);
   if (!format_desc) {
                  assert(format_desc->block.bits   <= UTIL_FORMAT_MAX_PACKED_BYTES * 8);
   assert(format_desc->block.height <= UTIL_FORMAT_MAX_UNPACKED_HEIGHT);
      #     define TEST_ONE_PACK_FUNC(name) \
         if (util_format_pack_description(format)->name) {                 \
      if (!test_one_func(format_desc, &test_format_##name, #name)) { \
   success = false; \
         #     define TEST_ONE_UNPACK_FUNC(name) \
         if (util_format_unpack_description(format)->name) {               \
      if (!test_one_func(format_desc, &test_format_##name, #name)) { \
   success = false; \
         #     define TEST_ONE_UNPACK_RECT_FUNC(name) \
         if (util_format_unpack_description(format)->name || util_format_unpack_description(format)->name##_rect) {               \
      if (!test_one_func(format_desc, &test_format_##name, #name)) { \
   success = false; \
         #     define TEST_FORMAT_METADATA(name) \
         if (!test_format_metadata(format_desc, &test_format_##name, #name)) { \
                  if (util_format_fetch_rgba_func(format)) {
      if (!test_one_func(format_desc, test_format_fetch_rgba, "fetch_rgba"))
               if (util_format_is_snorm(format)) {
               if (format == PIPE_FORMAT_R8G8Bx_SNORM) {
         } else if (unorm == format) {
      fprintf(stderr, "%s missing from util_format_snorm_to_unorm().\n",
            } else if (!util_format_is_unorm(unorm)) {
      fprintf(stderr, "util_format_snorm_to_unorm(%s) returned non-unorm %s.\n",
                        TEST_ONE_PACK_FUNC(pack_rgba_float);
   TEST_ONE_UNPACK_RECT_FUNC(unpack_rgba);
   TEST_ONE_PACK_FUNC(pack_rgba_8unorm);
            TEST_ONE_UNPACK_FUNC(unpack_z_32unorm);
   TEST_ONE_PACK_FUNC(pack_z_32unorm);
   TEST_ONE_UNPACK_FUNC(unpack_z_float);
   TEST_ONE_PACK_FUNC(pack_z_float);
   TEST_ONE_UNPACK_FUNC(unpack_s_8uint);
               #     undef TEST_ONE_FUNC
   #     undef TEST_ONE_FORMAT
                  }
         int main(int argc, char **argv)
   {
                           }
