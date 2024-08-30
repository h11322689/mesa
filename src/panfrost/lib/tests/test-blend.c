   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
      #include "pan_blend.h"
      /* A test consists of a given blend mode and its translated form */
   struct test {
      const char *label;
   struct pan_blend_equation eq;
   unsigned constant_mask;
   bool reads_dest;
   bool opaque;
   bool fixed_function;
   bool alpha_zero_nop;
   bool alpha_one_store;
      };
      /* clang-format off */
   #define RGBA(key, value) \
      .rgb_ ## key = value, \
         static const struct test blend_tests[] = {
      {
      "Replace",
   {
      .blend_enable = false,
      },
   .constant_mask = 0x0,
   .reads_dest = false,
   .opaque = true,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Alpha",
   {
                     RGBA(func, PIPE_BLEND_ADD),
   RGBA(src_factor, PIPE_BLENDFACTOR_SRC_ALPHA),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = true,
   .alpha_one_store = true,
      },
   {
      "Additive",
   {
                     RGBA(func, PIPE_BLEND_ADD),
   RGBA(src_factor, PIPE_BLENDFACTOR_ONE),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Additive-Alpha",
   {
                     RGBA(func, PIPE_BLEND_ADD),
   RGBA(src_factor, PIPE_BLENDFACTOR_SRC_ALPHA),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = true,
   .alpha_one_store = false,
      },
   {
      "Subtractive",
   {
                     RGBA(func, PIPE_BLEND_SUBTRACT),
   RGBA(src_factor, PIPE_BLENDFACTOR_ONE),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Subtractive-Alpha",
   {
                     RGBA(func, PIPE_BLEND_SUBTRACT),
   RGBA(src_factor, PIPE_BLENDFACTOR_SRC_ALPHA),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Modulate",
   {
                     RGBA(func, PIPE_BLEND_ADD),
   RGBA(src_factor, PIPE_BLENDFACTOR_ZERO),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Replace masked",
   {
      .blend_enable = false,
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Modulate masked",
   {
                     RGBA(func, PIPE_BLEND_ADD),
   RGBA(src_factor, PIPE_BLENDFACTOR_ZERO),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "src*dst + dst*src",
   {
                     RGBA(func, PIPE_BLEND_ADD),
   RGBA(src_factor, PIPE_BLENDFACTOR_DST_COLOR),
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Mixed src*dst + dst*src masked I",
   {
                     .rgb_func = PIPE_BLEND_ADD,
                  .alpha_func = PIPE_BLEND_ADD,
   .alpha_src_factor = PIPE_BLENDFACTOR_DST_COLOR,
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Mixed src*dst + dst*src masked II",
   {
                     .rgb_func = PIPE_BLEND_ADD,
                  .alpha_func = PIPE_BLEND_ADD,
   .alpha_src_factor = PIPE_BLENDFACTOR_DST_ALPHA,
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
      },
   {
      "Mixed src*dst + dst*src masked III",
   {
                     .rgb_func = PIPE_BLEND_ADD,
                  .alpha_func = PIPE_BLEND_ADD,
   .alpha_src_factor = PIPE_BLENDFACTOR_DST_ALPHA,
      },
   .constant_mask = 0x0,
   .reads_dest = true,
   .opaque = false,
   .fixed_function = true,
   .alpha_zero_nop = false,
   .alpha_one_store = false,
         };
   /* clang-format on */
      #define ASSERT_EQ(x, y)                                                        \
      do {                                                                        \
      if (x == y) {                                                            \
         } else {                                                                 \
      nr_fail++;                                                            \
   fprintf(stderr, "%s: Assertion failed %s (%x) != %s (%x)\n", T.label, \
               int
   main(int argc, const char **argv)
   {
               for (unsigned i = 0; i < ARRAY_SIZE(blend_tests); ++i) {
      struct test T = blend_tests[i];
   ASSERT_EQ(T.constant_mask, pan_blend_constant_mask(T.eq));
   ASSERT_EQ(T.reads_dest, pan_blend_reads_dest(T.eq));
   ASSERT_EQ(T.opaque, pan_blend_is_opaque(T.eq));
   ASSERT_EQ(T.fixed_function, pan_blend_can_fixed_function(T.eq, true));
   ASSERT_EQ(T.alpha_zero_nop, pan_blend_alpha_zero_nop(T.eq));
            if (pan_blend_can_fixed_function(T.eq, true)) {
                     printf("Passed %u/%u\n", nr_pass, nr_pass + nr_fail);
      }
