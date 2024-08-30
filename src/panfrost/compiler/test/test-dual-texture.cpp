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
      #include "bi_builder.h"
   #include "bi_test.h"
   #include "compiler.h"
      #include <gtest/gtest.h>
      #define CASE(shader_stage, instr, expected)                                    \
      do {                                                                        \
      bi_builder *A = bit_builder(mem_ctx);                                    \
   bi_builder *B = bit_builder(mem_ctx);                                    \
   {                                                                        \
      bi_builder *b = A;                                                    \
   bi_index u = bi_temp(b->shader);                                      \
   bi_index v = bi_temp(b->shader);                                      \
   A->shader->stage = MESA_SHADER_##shader_stage;                        \
      }                                                                        \
   {                                                                        \
      bi_builder *b = B;                                                    \
   bi_index u = bi_temp(b->shader);                                      \
   bi_index v = bi_temp(b->shader);                                      \
   B->shader->stage = MESA_SHADER_##shader_stage;                        \
      }                                                                        \
   bi_opt_fuse_dual_texture(A->shader);                                     \
   if (!bit_shader_equal(A->shader, B->shader)) {                           \
      ADD_FAILURE();                                                        \
   fprintf(stderr, "Optimization produce unexpected result");            \
   fprintf(stderr, "  Actual:\n");                                       \
   bi_print_shader(A->shader, stderr);                                   \
   fprintf(stderr, "Expected:\n");                                       \
   bi_print_shader(B->shader, stderr);                                   \
               #define NEGCASE(stage, instr) CASE(stage, instr, instr)
      class DualTexture : public testing::Test {
   protected:
      DualTexture()
   {
               reg = bi_register(0);
   x = bi_register(4);
               ~DualTexture()
   {
                              };
      TEST_F(DualTexture, FuseDualTexFragment)
   {
      CASE(
      FRAGMENT,
   {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF9F00144),
      }
      TEST_F(DualTexture, FuseDualTexKernel)
   {
      CASE(
      KERNEL,
   {
      bi_texs_2d_f32_to(b, x, u, v, true, 0, 0);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF9F00144), true,
      }
      TEST_F(DualTexture, FuseDualTexVertex)
   {
      CASE(
      VERTEX,
   {
      bi_texs_2d_f32_to(b, x, u, v, true, 0, 0);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF9F00144), true,
      }
      TEST_F(DualTexture, DontFuseDualTexWrongStage)
   {
      NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, u, v, true, 0, 0);
               NEGCASE(KERNEL, {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
               NEGCASE(VERTEX, {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
         }
      TEST_F(DualTexture, FuseDualTexMaximumIndex)
   {
      CASE(
      FRAGMENT,
   {
      bi_texs_2d_f32_to(b, x, u, v, false, 2, 2);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF9F003E6),
      }
      TEST_F(DualTexture, FuseDualTexMixedIndex)
   {
      CASE(
      FRAGMENT,
   {
      bi_texs_2d_f32_to(b, x, u, v, false, 3, 2);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF9F003A7),
      }
      TEST_F(DualTexture, DontFuseDualTexOutOfBounds)
   {
      NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, u, v, false, 4, 0);
               NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 4);
               NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
               NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
         }
      TEST_F(DualTexture, FuseDualTexFP16)
   {
      CASE(
      FRAGMENT,
   {
      bi_texs_2d_f16_to(b, x, u, v, false, 0, 0);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF1E00144),
      }
      TEST_F(DualTexture, FuseDualTexMixedSize)
   {
      CASE(
      FRAGMENT,
   {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0XF9E00144),
            CASE(
      FRAGMENT,
   {
      bi_texs_2d_f16_to(b, x, u, v, false, 0, 0);
      },
   {
      bi_texc_dual_to(b, x, y, bi_null(), u, v, bi_imm_u32(0xF1F00144),
      }
      TEST_F(DualTexture, DontFuseMixedCoordinates)
   {
      NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, bi_neg(u), v, false, 0, 0);
               NEGCASE(FRAGMENT, {
      bi_texs_2d_f32_to(b, x, u, v, false, 0, 0);
         }
