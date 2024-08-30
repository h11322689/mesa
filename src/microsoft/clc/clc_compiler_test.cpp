   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <cmath>
   #include <stdio.h>
   #include <stdint.h>
   #include <stdexcept>
   #include <vector>
      #include <unknwn.h>
   #include <directx/d3d12.h>
   #include <dxgi1_4.h>
   #include <gtest/gtest.h>
   #include <wrl.h>
   #include <dxguids/dxguids.h>
      #include "compute_test.h"
      using std::vector;
      TEST_F(ComputeTest, runtime_memcpy)
   {
      struct shift { uint8_t val; uint8_t shift; uint16_t ret; };
   const char *kernel_source =
   "struct shift { uchar val; uchar shift; ushort ret; };\n\
   __kernel void main_test(__global struct shift *inout)\n\
   {\n\
      uint id = get_global_id(0);\n\
   uint id2 = id + get_global_id(1);\n\
   struct shift lc[4] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }};\n\
   lc[id] = inout[id];\n\
               auto inout = ShaderArg<struct shift>({
         { 0x10, 1, 0xffff },
   { 0x20, 2, 0xffff },
   { 0x30, 3, 0xffff },
      },
      const uint16_t expected[] = { 0x20, 0x80, 0x180, 0x400 };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, two_global_arrays)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *g1, __global uint *g2)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   auto g1 = ShaderArg<uint32_t>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto g2 = ShaderArg<uint32_t>({ 1, 2, 3, 4 }, SHADER_ARG_INPUT);
   const uint32_t expected[] = {
                  run_shader(kernel_source, g1.size(), 1, 1, g1, g2);
   for (int i = 0; i < g1.size(); ++i)
      }
      TEST_F(ComputeTest, nested_arrays)
   {
         float4 DoMagic(float4 inValue)
   {
      const float testArr[3][3] = {
      {0.1f, 0.2f, 0.3f},
   {0.4f, 0.5f, 0.6f},
      float4 outValue = inValue;
   outValue.x = inValue.x * testArr[0][0] + inValue.y * testArr[0][1] + inValue.z * testArr[0][2];
   outValue.y = inValue.x * testArr[1][0] + inValue.y * testArr[1][1] + inValue.z * testArr[1][2];
   outValue.z = inValue.x * testArr[2][0] + inValue.y * testArr[2][1] + inValue.z * testArr[2][2];
      }
   __kernel void main_test(__global float4 *g1, __global float4 *g2)
   {
      uint idx = get_global_id(0);
      })";
      auto g1 = ShaderArg<float>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto g2 = ShaderArg<float>({ 0.2f, 0.4f, 0.6f, 1.0f }, SHADER_ARG_INPUT);
   const float expected[] = {
                  run_shader(kernel_source, 1, 1, 1, g1, g2);
   for (int i = 0; i < g1.size(); ++i)
      }
      /* Disabled until saturated conversions from f32->i64 fixed (mesa/mesa#3824) */
   TEST_F(ComputeTest, DISABLED_i64tof32)
   {
      const char *kernel_source =
   "__kernel void main_test(__global long *out, __constant long *in)\n\
   {\n\
      __local float tmp[12];\n\
   uint idx = get_global_id(0);\n\
   tmp[idx] = in[idx];\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
      }\n";
   auto in = ShaderArg<int64_t>({ 0x100000000LL,
                                 -0x100000000LL,
   0x7fffffffffffffffLL,
   0x4000004000000000LL,
   0x4000003fffffffffLL,
   0x4000004000000001LL,
   -1,
   -0x4000004000000000LL,
   auto out = ShaderArg<int64_t>(std::vector<int64_t>(12, 0xdeadbeed), SHADER_ARG_OUTPUT);
   const int64_t expected[] = {
      0x100000000LL,
   -0x100000000LL,
   0x7fffffffffffffffLL,
   0x4000000000000000LL,
   0x4000000000000000LL,
   0x4000008000000000LL,
   -1,
   -0x4000000000000000LL,
   -0x4000000000000000LL,
   -0x4000008000000000LL,
   0,
               run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
            }
   TEST_F(ComputeTest, two_constant_arrays)
   {
      const char *kernel_source =
   "__kernel void main_test(__constant uint *c1, __global uint *g1, __constant uint *c2)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   auto g1 = ShaderArg<uint32_t>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto c1 = ShaderArg<uint32_t>({ 1, 2, 3, 4 }, SHADER_ARG_INPUT);
   auto c2 = ShaderArg<uint32_t>(std::vector<uint32_t>(16384, 5), SHADER_ARG_INPUT);
   const uint32_t expected[] = {
                  run_shader(kernel_source, g1.size(), 1, 1, c1, g1, c2);
   for (int i = 0; i < g1.size(); ++i)
      }
      TEST_F(ComputeTest, null_constant_ptr)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *g1, __constant uint *c1)\n\
   {\n\
      __constant uint fallback[] = {2, 3, 4, 5};\n\
   __constant uint *c = c1 ? c1 : fallback;\n\
   uint idx = get_global_id(0);\n\
      }\n";
   auto g1 = ShaderArg<uint32_t>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto c1 = ShaderArg<uint32_t>({ 1, 2, 3, 4 }, SHADER_ARG_INPUT);
   const uint32_t expected1[] = {
                  run_shader(kernel_source, g1.size(), 1, 1, g1, c1);
   for (int i = 0; i < g1.size(); ++i)
            const uint32_t expected2[] = {
                  g1 = ShaderArg<uint32_t>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto c2 = NullShaderArg();
   run_shader(kernel_source, g1.size(), 1, 1, g1, c2);
   for (int i = 0; i < g1.size(); ++i)
      }
      TEST_F(ComputeTest, null_global_ptr)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *g1, __global uint *g2)\n\
   {\n\
      __constant uint fallback[] = {2, 3, 4, 5};\n\
   uint idx = get_global_id(0);\n\
      }\n";
   auto g1 = ShaderArg<uint32_t>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto g2 = ShaderArg<uint32_t>({ 1, 2, 3, 4 }, SHADER_ARG_INPUT);
   const uint32_t expected1[] = {
                  run_shader(kernel_source, g1.size(), 1, 1, g1, g2);
   for (int i = 0; i < g1.size(); ++i)
            const uint32_t expected2[] = {
                  g1 = ShaderArg<uint32_t>({ 10, 20, 30, 40 }, SHADER_ARG_INOUT);
   auto g2null = NullShaderArg();
   run_shader(kernel_source, g1.size(), 1, 1, g1, g2null);
   for (int i = 0; i < g1.size(); ++i)
      }
      TEST_F(ComputeTest, ret_constant_ptr)
   {
      struct s { uint64_t ptr; uint32_t val; };
   const char *kernel_source =
   "struct s { __constant uint *ptr; uint val; };\n\
   __kernel void main_test(__global struct s *out, __constant uint *in)\n\
   {\n\
      __constant uint foo[] = { 1, 2 };\n\
   uint idx = get_global_id(0);\n\
   if (idx == 0)\n\
         else\n\
            }\n";
   auto out = ShaderArg<struct s>(std::vector<struct s>(2, {0xdeadbeefdeadbeef, 0}), SHADER_ARG_OUTPUT);
   auto in = ShaderArg<uint32_t>({ 3, 4 }, SHADER_ARG_INPUT);
   const uint32_t expected_val[] = {
         };
   const uint64_t expected_ptr[] = {
                  run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].val, expected_val[i]);
         }
      TEST_F(ComputeTest, ret_global_ptr)
   {
      struct s { uint64_t ptr; uint32_t val; };
   const char *kernel_source =
   "struct s { __global uint *ptr; uint val; };\n\
   __kernel void main_test(__global struct s *out, __global uint *in1, __global uint *in2)\n\
   {\n\
      uint idx = get_global_id(0);\n\
   out[idx].ptr = idx ? in2 : in1;\n\
      }\n";
   auto out = ShaderArg<struct s>(std::vector<struct s>(2, {0xdeadbeefdeadbeef, 0}), SHADER_ARG_OUTPUT);
   auto in1 = ShaderArg<uint32_t>({ 1, 2 }, SHADER_ARG_INPUT);
   auto in2 = ShaderArg<uint32_t>({ 3, 4 }, SHADER_ARG_INPUT);
   const uint32_t expected_val[] = {
         };
   const uint64_t expected_ptr[] = {
                  run_shader(kernel_source, out.size(), 1, 1, out, in1, in2);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].val, expected_val[i]);
         }
      TEST_F(ComputeTest, ret_local_ptr)
   {
      struct s { uint64_t ptr; };
   const char *kernel_source =
   "struct s { __local uint *ptr; };\n\
   __kernel void main_test(__global struct s *out)\n\
   {\n\
      __local uint tmp[2];\n\
   uint idx = get_global_id(0);\n\
   tmp[idx] = idx;\n\
      }\n";
   auto out = ShaderArg<struct s>(std::vector<struct s>(2, { 0xdeadbeefdeadbeef }), SHADER_ARG_OUTPUT);
   const uint64_t expected_ptr[] = {
                  run_shader(kernel_source, out.size(), 1, 1, out);
   for (int i = 0; i < out.size(); ++i) {
            }
      TEST_F(ComputeTest, ret_private_ptr)
   {
      struct s { uint64_t ptr; uint32_t value; };
   const char *kernel_source =
   "struct s { __private uint *ptr; uint value; };\n\
   __kernel void main_test(__global struct s *out)\n\
   {\n\
      uint tmp[2] = {1, 2};\n\
   uint idx = get_global_id(0);\n\
   out[idx].ptr = &tmp[idx];\n\
      }\n";
   auto out = ShaderArg<struct s>(std::vector<struct s>(2, { 0xdeadbeefdeadbeef }), SHADER_ARG_OUTPUT);
   const uint64_t expected_ptr[] = {
         };
   const uint32_t expected_value[] = {
                  run_shader(kernel_source, out.size(), 1, 1, out);
   for (int i = 0; i < out.size(); ++i) {
            }
      TEST_F(ComputeTest, globals_8bit)
   {
      const char *kernel_source =
   "__kernel void main_test(__global unsigned char *inout)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<uint8_t> ({ 100, 110, 120, 130 }, SHADER_ARG_INOUT);
   const uint8_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, globals_16bit)
   {
      const char *kernel_source =
   "__kernel void main_test(__global unsigned short *inout)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<uint16_t> ({ 10000, 10010, 10020, 10030 }, SHADER_ARG_INOUT);
   const uint16_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, globals_64bit)
   {
      const char *kernel_source =
   "__kernel void main_test(__global unsigned long *inout)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   uint64_t base = 1ull << 50;
   auto inout = ShaderArg<uint64_t>({ base, base + 10, base + 20, base + 30 },
         const uint64_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, built_ins_global_id)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
                  run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, built_ins_global_id_rmw)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      uint id = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<uint32_t>({0x00000001, 0x10000001, 0x00020002, 0x04010203},
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, types_float_basics)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, DISABLED_types_double_basics)
   {
      /* Disabled because doubles are unsupported */
   const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, types_short_basics)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, types_char_basics)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, types_if_statement)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      int idx = get_global_id(0);\n\
   if (idx > 0)\n\
         else\n\
      }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, types_do_while_loop)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      int value = 1;\n\
   int i = 1, n = get_global_id(0);\n\
   do {\n\
         } while (i <= n);\n\
      }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(5, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, types_for_loop)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      int value = 1;\n\
   int n = get_global_id(0);\n\
   for (int i = 1; i <= n; ++i)\n\
            }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(5, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, complex_types_local_array_long)
   {
      const char *kernel_source =
   "__kernel void main_test(__global ulong *inout)\n\
   {\n\
      ulong tmp[] = {\n\
      get_global_id(1) + 0x00000000,\n\
   get_global_id(1) + 0x10000001,\n\
   get_global_id(1) + 0x20000020,\n\
      };\n\
   uint idx = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<uint64_t>({ 0, 0, 0, 0 }, SHADER_ARG_INOUT);
   const uint64_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, complex_types_local_array_short)
   {
      const char *kernel_source =
   "__kernel void main_test(__global ushort *inout)\n\
   {\n\
      ushort tmp[] = {\n\
      get_global_id(1) + 0x00,\n\
   get_global_id(1) + 0x10,\n\
   get_global_id(1) + 0x20,\n\
      };\n\
   uint idx = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<uint16_t>({ 0, 0, 0, 0 }, SHADER_ARG_INOUT);
   const uint16_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, complex_types_local_array_struct_vec_float_misaligned)
   {
      const char *kernel_source =
   "struct has_vecs { uchar c; ushort s; float2 f; };\n\
   __kernel void main_test(__global uint *inout)\n\
   {\n\
      struct has_vecs tmp[] = {\n\
      { 10 + get_global_id(0), get_global_id(1), { 10.0f, 1.0f } },\n\
   { 19 + get_global_id(0), get_global_id(1), { 20.0f, 4.0f } },\n\
   { 28 + get_global_id(0), get_global_id(1), { 30.0f, 9.0f } },\n\
      };\n\
   uint idx = get_global_id(0);\n\
   uint mul = (tmp[idx].c + tmp[idx].s) * trunc(tmp[idx].f[0]);\n\
      }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 0, 0, 0 }, SHADER_ARG_INOUT);
   const uint16_t expected[] = { 101, 404, 909, 1616 };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, complex_types_local_array)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
      uint tmp[] = {\n\
      get_global_id(1) + 0x00,\n\
   get_global_id(1) + 0x10,\n\
   get_global_id(1) + 0x20,\n\
      };\n\
   uint idx = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 0, 0, 0 }, SHADER_ARG_INOUT);
   const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, complex_types_global_struct_array)
   {
      struct two_vals { uint32_t add; uint32_t mul; };
   const char *kernel_source =
   "struct two_vals { uint add; uint mul; };\n\
   __kernel void main_test(__global struct two_vals *in_out)\n\
   {\n\
      uint id = get_global_id(0);\n\
   in_out[id].add = in_out[id].add + id;\n\
      }\n";
   auto inout = ShaderArg<struct two_vals>({ { 8, 8 }, { 16, 16 }, { 64, 64 }, { 65536, 65536 } },
         const struct two_vals expected[] = {
      { 8 + 0, 8 * 0 },
   { 16 + 1, 16 * 1 },
   { 64 + 2, 64 * 2 },
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i].add, expected[i].add);
         }
      TEST_F(ComputeTest, complex_types_global_uint2)
   {
      struct uint2 { uint32_t x; uint32_t y; };
   const char *kernel_source =
   "__kernel void main_test(__global uint2 *inout)\n\
   {\n\
      uint id = get_global_id(0);\n\
   inout[id].x = inout[id].x + id;\n\
      }\n";
   auto inout = ShaderArg<struct uint2>({ { 8, 8 }, { 16, 16 }, { 64, 64 }, { 65536, 65536 } },
         const struct uint2 expected[] = {
      { 8 + 0, 8 * 0 },
   { 16 + 1, 16 * 1 },
   { 64 + 2, 64 * 2 },
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i].x, expected[i].x);
         }
      TEST_F(ComputeTest, complex_types_global_ushort2)
   {
      struct ushort2 { uint16_t x; uint16_t y; };
   const char *kernel_source =
   "__kernel void main_test(__global ushort2 *inout)\n\
   {\n\
      uint id = get_global_id(0);\n\
   inout[id].x = inout[id].x + id;\n\
      }\n";
   auto inout = ShaderArg<struct ushort2>({ { 8, 8 }, { 16, 16 }, { 64, 64 },
               const struct ushort2 expected[] = {
      { 8 + 0, 8 * 0 },
   { 16 + 1, 16 * 1 },
   { 64 + 2, 64 * 2 },
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i].x, expected[i].x);
         }
      TEST_F(ComputeTest, complex_types_global_uchar3)
   {
      struct uchar3 { uint8_t x; uint8_t y; uint8_t z; uint8_t pad; };
   const char *kernel_source =
   "__kernel void main_test(__global uchar3 *inout)\n\
   {\n\
      uint id = get_global_id(0);\n\
   inout[id].x = inout[id].x + id;\n\
   inout[id].y = inout[id].y * id;\n\
      }\n";
   auto inout = ShaderArg<struct uchar3>({ { 8, 8, 8 }, { 16, 16, 16 }, { 64, 64, 64 }, { 255, 255, 255 } },
         const struct uchar3 expected[] = {
      { 8 + 0, 8 * 0, (8 + 0) + (8 * 0) },
   { 16 + 1, 16 * 1, (16 + 1) + (16 * 1) },
   { 64 + 2, 64 * 2, (64 + 2) + (64 * 2) },
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i].x, expected[i].x);
   EXPECT_EQ(inout[i].y, expected[i].y);
         }
      TEST_F(ComputeTest, complex_types_constant_uchar3)
   {
      struct uchar3 { uint8_t x; uint8_t y; uint8_t z; uint8_t pad; };
   const char *kernel_source =
   "__kernel void main_test(__global uchar3 *out, __constant uchar3 *in)\n\
   {\n\
      uint id = get_global_id(0);\n\
   out[id].x = in[id].x + id;\n\
   out[id].y = in[id].y * id;\n\
      }\n";
   auto in = ShaderArg<struct uchar3>({ { 8, 8, 8 }, { 16, 16, 16 }, { 64, 64, 64 }, { 255, 255, 255 } },
         auto out = ShaderArg<struct uchar3>(std::vector<struct uchar3>(4, { 0xff, 0xff, 0xff }),
         const struct uchar3 expected[] = {
      { 8 + 0, 8 * 0, (8 + 0) + (8 * 0) },
   { 16 + 1, 16 * 1, (16 + 1) + (16 * 1) },
   { 64 + 2, 64 * 2, (64 + 2) + (64 * 2) },
      };
   run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].x, expected[i].x);
   EXPECT_EQ(out[i].y, expected[i].y);
         }
      TEST_F(ComputeTest, complex_types_global_uint8)
   {
      struct uint8 {
      uint32_t s0; uint32_t s1; uint32_t s2; uint32_t s3;
      };
   const char *kernel_source =
   "__kernel void main_test(__global uint8 *inout)\n\
   {\n\
      uint id = get_global_id(0);\n\
      }\n";
   auto inout = ShaderArg<struct uint8>({ { 1, 2, 3, 4, 5, 6, 7, 8 } },
         const struct uint8 expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i].s0, expected[i].s0);
   EXPECT_EQ(inout[i].s1, expected[i].s1);
   EXPECT_EQ(inout[i].s2, expected[i].s2);
   EXPECT_EQ(inout[i].s3, expected[i].s3);
   EXPECT_EQ(inout[i].s4, expected[i].s4);
   EXPECT_EQ(inout[i].s5, expected[i].s5);
   EXPECT_EQ(inout[i].s6, expected[i].s6);
         }
      TEST_F(ComputeTest, complex_types_local_ulong16)
   {
      struct ulong16 {
         };
   const char *kernel_source =
   R"(__kernel void main_test(__global ulong16 *inout)
   {
      __local ulong16 local_array[2];
   uint id = get_global_id(0);
   local_array[id] = inout[id];
   barrier(CLK_LOCAL_MEM_FENCE);
      })";
   auto inout = ShaderArg<struct ulong16>({ { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } },
         const struct ulong16 expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      for (int j = 0; j < 16; ++j) {
               }
      TEST_F(ComputeTest, complex_types_constant_uint8)
   {
      struct uint8 {
      uint32_t s0; uint32_t s1; uint32_t s2; uint32_t s3;
      };
   const char *kernel_source =
   "__kernel void main_test(__global uint8 *out, __constant uint8 *in)\n\
   {\n\
      uint id = get_global_id(0);\n\
      }\n";
   auto in = ShaderArg<struct uint8>({ { 1, 2, 3, 4, 5, 6, 7, 8 } },
         auto out = ShaderArg<struct uint8>({ { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } },
         const struct uint8 expected[] = {
         };
   run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].s0, expected[i].s0);
   EXPECT_EQ(out[i].s1, expected[i].s1);
   EXPECT_EQ(out[i].s2, expected[i].s2);
   EXPECT_EQ(out[i].s3, expected[i].s3);
   EXPECT_EQ(out[i].s4, expected[i].s4);
   EXPECT_EQ(out[i].s5, expected[i].s5);
   EXPECT_EQ(out[i].s6, expected[i].s6);
         }
      TEST_F(ComputeTest, complex_types_const_array)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      const uint foo[] = { 100, 101, 102, 103 };\n\
      }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, mem_access_load_store_ordering)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      uint foo[4];\n\
   foo[0] = 0x11111111;\n\
   foo[1] = 0x22222222;\n\
   foo[2] = 0x44444444;\n\
   foo[3] = 0x88888888;\n\
   foo[get_global_id(1)] -= 0x11111111; // foo[0] = 0 \n\
   foo[0] += get_global_id(0); // foo[0] = tid\n\
   foo[foo[get_global_id(1)]] = get_global_id(0); // foo[tid] = tid\n\
      }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint16_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, two_const_arrays)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *output)\n\
   {\n\
      uint id = get_global_id(0);\n\
   uint foo[4] = {100, 101, 102, 103};\n\
   uint bar[4] = {1, 2, 3, 4};\n\
      }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, imod_pos)
   {
      const char *kernel_source =
   "__kernel void main_test(__global int *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<int32_t>({ -4, -3, -2, -1, 0, 1, 2, 3, 4 },
         const int32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, imod_neg)
   {
      const char *kernel_source =
   "__kernel void main_test(__global int *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<int32_t>({ -4, -3, -2, -1, 0, 1, 2, 3, 4 },
         const int32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, umod)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0xfffffffa, 0xfffffffb, 0xfffffffc, 0xfffffffd, 0xfffffffe },
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, rotate)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, popcount)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 0x1, 0x3, 0x101, 0x110011, ~0u },
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, hadd)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 1, 2, 3, 0xfffffffc, 0xfffffffd, 0xfffffffe, 0xffffffff },
         const uint32_t expected[] = {
      (1u << 31) >> 1,
   ((1u << 31) + 1) >> 1,
   ((1u << 31) + 2) >> 1,
   ((1u << 31) + 3) >> 1,
   ((1ull << 31) + 0xfffffffc) >> 1,
   ((1ull << 31) + 0xfffffffd) >> 1,
   ((1ull << 31) + 0xfffffffe) >> 1,
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, rhadd)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 1, 2, 3, 0xfffffffc, 0xfffffffd, 0xfffffffe, 0xffffffff },
         const uint32_t expected[] = {
      ((1u << 31) + 1) >> 1,
   ((1u << 31) + 2) >> 1,
   ((1u << 31) + 3) >> 1,
   ((1u << 31) + 4) >> 1,
   ((1ull << 31) + 0xfffffffd) >> 1,
   ((1ull << 31) + 0xfffffffe) >> 1,
   ((1ull << 31) + 0xffffffff) >> 1,
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, add_sat)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0xffffffff - 3, 0xffffffff - 2, 0xffffffff - 1, 0xffffffff },
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, sub_sat)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 1, 2, 3 }, SHADER_ARG_INOUT);
   const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, mul_hi)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 1, 2, 3, (1u << 31) }, SHADER_ARG_INOUT);
   const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, ldexp_x)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 0.5f, 1.0f, 2.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, ldexp_y)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.25f, 0.5f, 0.75f, 1.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, frexp_ret)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
      int exp;\n\
      }\n";
   auto inout = ShaderArg<float>({ 0.0f, 0.5f, 1.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, frexp_exp)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
      int exp;\n\
   frexp(inout[get_global_id(0)], &exp);\n\
      }\n";
   auto inout = ShaderArg<float>({ 0.0f, 0.5f, 1.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, clz)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<uint32_t>({ 0, 1, 0xffff,  (1u << 30), (1u << 31) }, SHADER_ARG_INOUT);
   const uint32_t expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, sin)
   {
      struct sin_vals { float in; float clc; float native; };
   const char *kernel_source =
   "struct sin_vals { float in; float clc; float native; };\n\
   __kernel void main_test(__global struct sin_vals *inout)\n\
   {\n\
      inout[get_global_id(0)].clc = sin(inout[get_global_id(0)].in);\n\
      }\n";
   const vector<sin_vals> input = {
      { 0.0f, 0.0f, 0.0f },
   { 1.0f, 0.0f, 0.0f },
   { 2.0f, 0.0f, 0.0f },
      };
   auto inout = ShaderArg<sin_vals>(input, SHADER_ARG_INOUT);
   const struct sin_vals expected[] = {
      { 0.0f, 0.0f,       0.0f       },
   { 1.0f, sin(1.0f), sin(1.0f) },
   { 2.0f, sin(2.0f), sin(2.0f) },
      };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_FLOAT_EQ(inout[i].in, inout[i].in);
   EXPECT_FLOAT_EQ(inout[i].clc, inout[i].clc);
         }
      TEST_F(ComputeTest, cosh)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, exp)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, exp10)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, exp2)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, log)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, log10)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, log2)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0.0f, 1.0f, 2.0f, 3.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, rint)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
                  auto inout = ShaderArg<float>({ 0.5f, 1.5f, -0.5f, -1.5f, 1.4f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, round)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0, 0.3f, -0.3f, 0.5f, -0.5f, 1.1f, -1.1f },
         const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, arg_by_val)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout, float mul)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0, 0.3f, -0.3f, 0.5f, -0.5f, 1.1f, -1.1f },
         auto mul = ShaderArg<float>(10.0f, SHADER_ARG_INPUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout, mul);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, uint8_by_val)
   {
      struct uint8 {
      uint32_t s0; uint32_t s1; uint32_t s2; uint32_t s3;
      };
   const char *kernel_source =
   "__kernel void main_test(__global uint *out, uint8 val)\n\
   {\n\
      out[get_global_id(0)] = val.s0 + val.s1 + val.s2 + val.s3 +\n\
      }\n";
   auto out = ShaderArg<uint32_t>({ 0 }, SHADER_ARG_OUTPUT);
   auto val = ShaderArg<struct uint8>({ {0, 1, 2, 3, 4, 5, 6, 7 }}, SHADER_ARG_INPUT);
   const uint32_t expected[] = { 0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 };
   run_shader(kernel_source, out.size(), 1, 1, out, val);
   for (int i = 0; i < out.size(); ++i)
      }
      TEST_F(ComputeTest, link)
   {
      const char *foo_src =
   "float foo(float in)\n\
   {\n\
         }\n";
   const char *kernel_source =
   "float foo(float in);\n\
   __kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   std::vector<const char *> srcs = { foo_src, kernel_source };
   auto inout = ShaderArg<float>({ 2.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(srcs, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, link_library)
   {
      const char *bar_src =
   "float bar(float in)\n\
   {\n\
         }\n";
   const char *foo_src =
   "float bar(float in);\n\
   float foo(float in)\n\
   {\n\
         }\n";
   const char *kernel_source =
   "float foo(float in);\n\
   __kernel void main_test(__global float *inout)\n\
   {\n\
         }\n";
   std::vector<Shader> libraries = {
      compile({ bar_src, kernel_source }, {}, true),
      };
   Shader exe = link(libraries);
   auto inout = ShaderArg<float>({ 2.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(exe, { (unsigned)inout.size(), 1, 1 }, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, localvar)
   {
      const char *kernel_source =
   "__kernel __attribute__((reqd_work_group_size(2, 1, 1)))\n\
   void main_test(__global float *inout)\n\
   {\n\
      __local float2 tmp[2];\n\
   tmp[get_local_id(0)].x = inout[get_global_id(0)] + 1;\n\
   tmp[get_local_id(0)].y = inout[get_global_id(0)] - 1;\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
               auto inout = ShaderArg<float>({ 2.0f, 4.0f }, SHADER_ARG_INOUT);
   const float expected[] = {
         };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, localvar_uchar2)
   {
      const char *kernel_source =
   "__attribute__((reqd_work_group_size(2, 1, 1)))\n\
   __kernel void main_test(__global uchar *inout)\n\
   {\n\
      __local uchar2 tmp[2];\n\
   tmp[get_local_id(0)].x = inout[get_global_id(0)] + 1;\n\
   tmp[get_local_id(0)].y = inout[get_global_id(0)] - 1;\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
               auto inout = ShaderArg<uint8_t>({ 2, 4 }, SHADER_ARG_INOUT);
   const uint8_t expected[] = { 9, 5 };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, work_group_size_hint)
   {
      const char *kernel_source =
   "__attribute__((work_group_size_hint(2, 1, 1)))\n\
   __kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, reqd_work_group_size)
   {
      const char *kernel_source =
   "__attribute__((reqd_work_group_size(2, 1, 1)))\n\
   __kernel void main_test(__global uint *output)\n\
   {\n\
         }\n";
   auto output = ShaderArg<uint32_t>(std::vector<uint32_t>(4, 0xdeadbeef),
         const uint32_t expected[] = {
         };
   run_shader(kernel_source, output.size(), 1, 1, output);
   for (int i = 0; i < output.size(); ++i)
      }
      TEST_F(ComputeTest, image)
   {
      const char* kernel_source =
   "__kernel void main_test(read_only image2d_t input, write_only image2d_t output)\n\
   {\n\
      int2 coords = (int2)(get_global_id(0), get_global_id(1));\n\
      }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, image_two_reads)
   {
      const char* kernel_source =
   "__kernel void main_test(image2d_t image, int is_float, __global float* output)\n\
   {\n\
      if (is_float)\n\
         else \n\
      }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, image_unused)
   {
      const char* kernel_source =
   "__kernel void main_test(read_only image2d_t input, write_only image2d_t output)\n\
   {\n\
   }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, image_read_write)
   {
      const char *kernel_source =
   R"(__kernel void main_test(read_write image2d_t image)
   {
      int2 coords = (int2)(get_global_id(0), get_global_id(1));
      })";
   Shader shader = compile(std::vector<const char*>({ kernel_source }), { "-cl-std=cl3.0" });
      }
      TEST_F(ComputeTest, sampler)
   {
      const char* kernel_source =
   "__kernel void main_test(image2d_t image, sampler_t sampler, __global float* output)\n\
   {\n\
         }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, image_dims)
   {
      const char* kernel_source =
   "__kernel void main_test(image2d_t roimage, write_only image2d_t woimage, __global uint* output)\n\
   {\n\
      output[get_global_id(0)] = get_image_width(roimage);\n\
      }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, image_format)
   {
      const char* kernel_source =
   "__kernel void main_test(image2d_t roimage, write_only image2d_t woimage, __global uint* output)\n\
   {\n\
      output[get_global_id(0)] = get_image_channel_data_type(roimage);\n\
      }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, image1d_buffer_t)
   {
      const char* kernel_source =
   "__kernel void main_test(read_only image1d_buffer_t input, write_only image1d_buffer_t output)\n\
   {\n\
         }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, local_ptr)
   {
      struct uint2 { uint32_t x, y; };
   const char *kernel_source =
   "__kernel void main_test(__global uint *inout, __local uint2 *tmp)\n\
   {\n\
      tmp[get_local_id(0)].x = inout[get_global_id(0)] + 1;\n\
   tmp[get_local_id(0)].y = inout[get_global_id(0)] - 1;\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
      }\n";
   auto inout = ShaderArg<uint32_t>({ 2, 4 }, SHADER_ARG_INOUT);
   auto tmp = ShaderArg<struct uint2>(std::vector<struct uint2>(4096), SHADER_ARG_INPUT);
   const uint8_t expected[] = { 9, 5 };
   run_shader(kernel_source, inout.size(), 1, 1, inout, tmp);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, two_local_ptrs)
   {
      struct uint2 { uint32_t x, y; };
   const char *kernel_source =
   "__kernel void main_test(__global uint *inout, __local uint2 *tmp, __local uint *tmp2)\n\
   {\n\
      tmp[get_local_id(0)].x = inout[get_global_id(0)] + 1;\n\
   tmp[get_local_id(0)].y = inout[get_global_id(0)] - 1;\n\
   tmp2[get_local_id(0)] = get_global_id(0);\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
      }\n";
   auto inout = ShaderArg<uint32_t>({ 2, 4 }, SHADER_ARG_INOUT);
   auto tmp = ShaderArg<struct uint2>(std::vector<struct uint2>(1024), SHADER_ARG_INPUT);
   auto tmp2 = ShaderArg<uint32_t>(std::vector<uint32_t>(1024), SHADER_ARG_INPUT);
   const uint8_t expected[] = { 9, 6 };
   run_shader(kernel_source, inout.size(), 1, 1, inout, tmp, tmp2);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, int8_to_float)
   {
      const char *kernel_source =
   "__kernel void main_test(__global char* in, __global float* out)\n\
   {\n\
      uint pos = get_global_id(0);\n\
      }";
   auto in = ShaderArg<char>({ 10, 20, 30, 40 }, SHADER_ARG_INPUT);
   auto out = ShaderArg<float>(std::vector<float>(4, std::numeric_limits<float>::infinity()), SHADER_ARG_OUTPUT);
   const float expected[] = { 0.1f, 0.2f, 0.3f, 0.4f };
   run_shader(kernel_source, in.size(), 1, 1, in, out);
   for (int i = 0; i < in.size(); ++i)
      }
      TEST_F(ComputeTest, vec_hint_float4)
   {
      const char *kernel_source =
   "__kernel __attribute__((vec_type_hint(float4))) void main_test(__global float *inout)\n\
   {\n\
         }";
   Shader shader = compile({ kernel_source });
   EXPECT_EQ(shader.metadata->kernels[0].vec_hint_size, 4);
      }
      TEST_F(ComputeTest, vec_hint_uchar2)
   {
      const char *kernel_source =
   "__kernel __attribute__((vec_type_hint(uchar2))) void main_test(__global float *inout)\n\
   {\n\
         }";
   Shader shader = compile({ kernel_source });
   EXPECT_EQ(shader.metadata->kernels[0].vec_hint_size, 2);
      }
      TEST_F(ComputeTest, vec_hint_none)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *inout)\n\
   {\n\
         }";
   Shader shader = compile({ kernel_source });
      }
      TEST_F(ComputeTest, DISABLED_debug_layer_failure)
   {
      /* This is a negative test case, it intentionally triggers a failure to validate the mechanism
   * is in place, so other tests will fail if they produce debug messages
   */
   const char *kernel_source =
   "__kernel void main_test(__global float *inout, float mul)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<float>({ 0, 0.3f, -0.3f, 0.5f, -0.5f, 1.1f, -1.1f },
         auto mul = ShaderArg<float>(10.0f, SHADER_ARG_INPUT);
   const float expected[] = {
         };
   ComPtr<ID3D12InfoQueue> info_queue;
   dev->QueryInterface(info_queue.ReleaseAndGetAddressOf());
   if (!info_queue) {
      GTEST_SKIP() << "No info queue";
               info_queue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_ERROR, "This should cause the test to fail");
   run_shader(kernel_source, inout.size(), 1, 1, inout, mul);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, compiler_defines)
   {
      const char *kernel_source =
         {\n\
      out[0] = OUT_VAL0;\n\
      }";
   auto out = ShaderArg<int>(std::vector<int>(2, 0), SHADER_ARG_OUTPUT);
   CompileArgs compile_args = { 1, 1, 1 };
   compile_args.compiler_command_line = { "-DOUT_VAL0=5", "-cl-std=cl" };
   std::vector<RawShaderArg *> raw_args = { &out };
   run_shader({ kernel_source }, compile_args, out);
   EXPECT_EQ(out[0], 5);
      }
      TEST_F(ComputeTest, global_atomic_add)
   {
      const char *kernel_source =
   "__kernel void main_test(__global int *inout, __global int *old)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<int32_t>({ 2, 4 }, SHADER_ARG_INOUT);
   auto old = ShaderArg<int32_t>(std::vector<int32_t>(2, 0xdeadbeef), SHADER_ARG_OUTPUT);
   const int32_t expected_inout[] = { 5, 7 };
   const int32_t expected_old[] = { 2, 4 };
   run_shader(kernel_source, inout.size(), 1, 1, inout, old);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i], expected_inout[i]);
         }
      TEST_F(ComputeTest, global_atomic_imin)
   {
      const char *kernel_source =
   "__kernel void main_test(__global int *inout, __global int *old)\n\
   {\n\
         }\n";
   auto inout = ShaderArg<int32_t>({ 0, 2, -1 }, SHADER_ARG_INOUT);
   auto old = ShaderArg<int32_t>(std::vector<int32_t>(3, 0xdeadbeef), SHADER_ARG_OUTPUT);
   const int32_t expected_inout[] = { 0, 1, -1 };
   const int32_t expected_old[] = { 0, 2, -1 };
   run_shader(kernel_source, inout.size(), 1, 1, inout, old);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i], expected_inout[i]);
         }
      TEST_F(ComputeTest, global_atomic_and_or)
   {
      const char *kernel_source =
   "__attribute__((reqd_work_group_size(3, 1, 1)))\n\
   __kernel void main_test(__global int *inout)\n\
   {\n\
      atomic_and(inout, ~(1 << get_global_id(0)));\n\
      }\n";
   auto inout = ShaderArg<int32_t>(0xf, SHADER_ARG_INOUT);
   const int32_t expected[] = { 0x78 };
   run_shader(kernel_source, 3, 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, global_atomic_cmpxchg)
   {
      const char *kernel_source =
   "__attribute__((reqd_work_group_size(2, 1, 1)))\n\
   __kernel void main_test(__global int *inout)\n\
   {\n\
      while (atomic_cmpxchg(inout, get_global_id(0), get_global_id(0) + 1) != get_global_id(0))\n\
      }\n";
   auto inout = ShaderArg<int32_t>(0, SHADER_ARG_INOUT);
   const int32_t expected_inout[] = { 2 };
   run_shader(kernel_source, 2, 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, local_atomic_and_or)
   {
      const char *kernel_source =
   "__attribute__((reqd_work_group_size(2, 1, 1)))\n\
   __kernel void main_test(__global ushort *inout)\n\
   {\n\
      __local ushort tmp;\n\
   atomic_and(&tmp, ~(0xff << (get_global_id(0) * 8)));\n\
   atomic_or(&tmp, inout[get_global_id(0)] << (get_global_id(0) * 8));\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
      }\n";
   auto inout = ShaderArg<uint16_t>({ 2, 4 }, SHADER_ARG_INOUT);
   const uint16_t expected[] = { 0x402, 0x402 };
   run_shader(kernel_source, inout.size(), 1, 1, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, local_atomic_cmpxchg)
   {
      const char *kernel_source =
   "__attribute__((reqd_work_group_size(2, 1, 1)))\n\
   __kernel void main_test(__global int *out)\n\
   {\n\
      __local uint tmp;\n\
   tmp = 0;\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
   while (atomic_cmpxchg(&tmp, get_global_id(0), get_global_id(0) + 1) != get_global_id(0))\n\
         barrier(CLK_LOCAL_MEM_FENCE);\n\
               auto out = ShaderArg<uint32_t>(0xdeadbeef, SHADER_ARG_OUTPUT);
   const uint16_t expected[] = { 2 };
   run_shader(kernel_source, 2, 1, 1, out);
   for (int i = 0; i < out.size(); ++i)
      }
      TEST_F(ComputeTest, constant_sampler)
   {
      const char* kernel_source =
   "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;\n\
   __kernel void main_test(read_only image2d_t input, write_only image2d_t output)\n\
   {\n\
      int2 coordsi = (int2)(get_global_id(0), get_global_id(1));\n\
   float2 coordsf = (float2)((float)coordsi.x / get_image_width(input), (float)coordsi.y / get_image_height(input));\n\
   write_imagef(output, coordsi, \n\
      read_imagef(input, sampler, coordsf) + \n\
   }\n";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
   validate(shader);
      }
      TEST_F(ComputeTest, hi)
   {
      const char *kernel_source = R"(
   __kernel void main_test(__global char3 *srcA, __global char2 *dst)
   {
               char2 tmp = srcA[tid].hi;
      })";
   Shader shader = compile(std::vector<const char*>({ kernel_source }));
      }
      TEST_F(ComputeTest, system_values)
   {
      const char *kernel_source =
   "__kernel void main_test(__global uint* outputs)\n\
   {\n\
      outputs[0] = get_work_dim();\n\
   outputs[1] = get_global_size(0);\n\
   outputs[2] = get_local_size(0);\n\
   outputs[3] = get_num_groups(0);\n\
   outputs[4] = get_group_id(0);\n\
   outputs[5] = get_global_offset(0);\n\
      }\n";
   auto out = ShaderArg<uint32_t>(std::vector<uint32_t>(6, 0xdeadbeef), SHADER_ARG_OUTPUT);
   const uint16_t expected[] = { 3, 1, 1, 1, 0, 0, 0, };
   CompileArgs args = { 1, 1, 1 };
   Shader shader = compile({ kernel_source });
   run_shader(shader, args, out);
   for (int i = 0; i < out.size(); ++i)
            args.work_props.work_dim = 2;
   args.work_props.global_offset_x = 100;
   args.work_props.group_id_offset_x = 2;
   args.work_props.group_count_total_x = 5;
   const uint32_t expected_withoffsets[] = { 2, 5, 1, 5, 2, 100, 102 };
   run_shader(shader, args, out);
   for (int i = 0; i < out.size(); ++i)
      }
      TEST_F(ComputeTest, convert_round_sat)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float *f, __global uchar *u)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   auto f = ShaderArg<float>({ -1.0f, 1.1f, 20.0f, 255.5f }, SHADER_ARG_INPUT);
   auto u = ShaderArg<uint8_t>({ 255, 0, 0, 0 }, SHADER_ARG_OUTPUT);
   const uint8_t expected[] = {
                  run_shader(kernel_source, f.size(), 1, 1, f, u);
   for (int i = 0; i < u.size(); ++i)
      }
      TEST_F(ComputeTest, convert_round_sat_vec)
   {
      const char *kernel_source =
   "__kernel void main_test(__global float16 *f, __global uchar16 *u)\n\
   {\n\
      uint idx = get_global_id(0);\n\
      }\n";
   auto f = ShaderArg<float>({
      -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f,
   -0.5f, 1.9f, 20.0f, 254.5f, -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f,
   0.0f, 1.3f, 20.0f, 255.1f, -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f, -1.0f, 1.1f, 20.0f, 255.5f,
      }, SHADER_ARG_INPUT);
   auto u = ShaderArg<uint8_t>({
      255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0,
   255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0,
   255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0,
      }, SHADER_ARG_OUTPUT);
   const uint8_t expected[] = {
      0, 2, 20, 255, 0, 2, 20, 255, 0, 2, 20, 255, 0, 2, 20, 255,
   0, 2, 20, 255, 0, 2, 20, 255, 0, 2, 20, 255, 0, 2, 20, 255,
   0, 2, 20, 255, 0, 2, 20, 255, 0, 2, 20, 255, 0, 2, 20, 255,
               run_shader(kernel_source, 4, 1, 1, f, u);
   for (int i = 0; i < u.size(); ++i)
      }
      TEST_F(ComputeTest, convert_char2_uchar2)
   {
      const char *kernel_source =
   "__kernel void main_test( __global char2 *src, __global uchar2 *dest )\n\
   {\n\
      size_t i = get_global_id(0);\n\
               auto c = ShaderArg<int8_t>({ -127, -4, 0, 4, 126, 127, 16, 32 }, SHADER_ARG_INPUT);
   auto u = ShaderArg<uint8_t>({ 99, 99, 99, 99, 99, 99, 99, 99 }, SHADER_ARG_OUTPUT);
   const uint8_t expected[] = { 0, 0, 0, 4, 126, 127, 16, 32 };
   run_shader(kernel_source, 4, 1, 1, c, u);
   for (int i = 0; i < u.size(); i++)
      }
      TEST_F(ComputeTest, async_copy)
   {
      const char *kernel_source = R"(
   __kernel void main_test( const __global char *src, __global char *dst, __local char *localBuffer, int copiesPerWorkgroup, int copiesPerWorkItem )
   {
   int i;
   for(i=0; i<copiesPerWorkItem; i++)
      localBuffer[ get_local_id( 0 )*copiesPerWorkItem+i ] = (char)(char)0;
   barrier( CLK_LOCAL_MEM_FENCE );
   event_t event;
   event = async_work_group_copy( (__local char*)localBuffer, (__global const char*)(src+copiesPerWorkgroup*get_group_id(0)), (size_t)copiesPerWorkgroup, 0 );
      for(i=0; i<copiesPerWorkItem; i++)
   dst[ get_global_id( 0 )*copiesPerWorkItem+i ] = localBuffer[ get_local_id( 0 )*copiesPerWorkItem+i ];
   })";
   Shader shader = compile({ kernel_source });
      }
      TEST_F(ComputeTest, packed_struct_global)
   {
   #pragma pack(push, 1)
         #pragma pack(pop)
         const char *kernel_source =
   "struct __attribute__((packed)) s {uchar uc; ulong ul; ushort us; };\n\
   __kernel void main_test(__global struct s *inout, global uint *size)\n\
   {\n\
      uint idx = get_global_id(0);\n\
   inout[idx].uc = idx + 1;\n\
   inout[idx].ul = ((ulong)(idx + 1 + 0xfbfcfdfe) << 32) | 0x12345678;\n\
   inout[idx].us = ((ulong)(idx + 1 + 0xa0) << 8) | 0x12;\n\
      }\n";
   auto inout = ShaderArg<struct s>({0, 0, 0}, SHADER_ARG_OUTPUT);
   auto size = ShaderArg<uint32_t>(0, SHADER_ARG_OUTPUT);
   const struct s expected[] = {
                  run_shader(kernel_source, inout.size(), 1, 1, inout, size);
   for (int i = 0; i < inout.size(); ++i) {
      EXPECT_EQ(inout[i].uc, expected[i].uc);
   EXPECT_EQ(inout[i].ul, expected[i].ul);
      }
      }
      TEST_F(ComputeTest, packed_struct_arg)
   {
   #pragma pack(push, 1)
         #pragma pack(pop)
         const char *kernel_source =
   "struct __attribute__((packed)) s {uchar uc; ulong ul; ushort us; };\n\
   __kernel void main_test(__global struct s *out, struct s in)\n\
   {\n\
      uint idx = get_global_id(0);\n\
   out[idx].uc = in.uc + 0x12;\n\
   out[idx].ul = in.ul + 0x123456789abcdef;\n\
      }\n";
   auto out = ShaderArg<struct s>({0, 0, 0}, SHADER_ARG_OUTPUT);
   auto in = ShaderArg<struct s>({1, 2, 3}, SHADER_ARG_INPUT);
   const struct s expected[] = {
                  run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].uc, expected[i].uc);
   EXPECT_EQ(out[i].ul, expected[i].ul);
         }
      TEST_F(ComputeTest, packed_struct_local)
   {
   #pragma pack(push, 1)
         #pragma pack(pop)
         const char *kernel_source =
   "struct __attribute__((packed)) s {uchar uc; ulong ul; ushort us; };\n\
   __kernel void main_test(__global struct s *out, __constant struct s *in)\n\
   {\n\
      uint idx = get_global_id(0);\n\
   __local struct s tmp[2];\n\
   tmp[get_local_id(0)] = in[idx];\n\
   barrier(CLK_LOCAL_MEM_FENCE);\n\
      }\n";
   auto out = ShaderArg<struct s>({{0, 0, 0}, {0, 0, 0}}, SHADER_ARG_OUTPUT);
   auto in = ShaderArg<struct s>({{1, 2, 3}, {0x12, 0x123456789abcdef, 0x1234} }, SHADER_ARG_INPUT);
   const struct s expected[] = {
      { 0x12, 0x123456789abcdef, 0x1234 },
               run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].uc, expected[i].uc);
   EXPECT_EQ(out[i].ul, expected[i].ul);
         }
      TEST_F(ComputeTest, packed_struct_const)
   {
   #pragma pack(push, 1)
         #pragma pack(pop)
         const char *kernel_source =
   "struct __attribute__((packed)) s {uchar uc; ulong ul; ushort us; };\n\
   __kernel void main_test(__global struct s *out, struct s in)\n\
   {\n\
      __constant struct s base[] = {\n\
      {0x12, 0x123456789abcdef, 0x1234},\n\
      };\n\
   uint idx = get_global_id(0);\n\
   out[idx].uc = base[idx % 2].uc + in.uc;\n\
   out[idx].ul = base[idx % 2].ul + in.ul;\n\
      }\n";
   auto out = ShaderArg<struct s>(std::vector<struct s>(2, {0, 0, 0}), SHADER_ARG_OUTPUT);
   auto in = ShaderArg<struct s>({1, 2, 3}, SHADER_ARG_INPUT);
   const struct s expected[] = {
      { 0x12 + 1, 0x123456789abcdef + 2, 0x1234 + 3 },
               run_shader(kernel_source, out.size(), 1, 1, out, in);
   for (int i = 0; i < out.size(); ++i) {
      EXPECT_EQ(out[i].uc, expected[i].uc);
   EXPECT_EQ(out[i].ul, expected[i].ul);
         }
      TEST_F(ComputeTest, printf)
   {
      const char *kernel_source = R"(
   __kernel void main_test(__global float *src, __global uint *dest)
   {
                  auto src = ShaderArg<float>({ 1.0f }, SHADER_ARG_INPUT);
   auto dest = ShaderArg<uint32_t>({ 0xdeadbeef }, SHADER_ARG_OUTPUT);
   run_shader(kernel_source, 1, 1, 1, src, dest);
      }
      TEST_F(ComputeTest, vload_half)
   {
      const char *kernel_source = R"(
   __kernel void main_test(__global half *src, __global float4 *dest)
   {
      int offset = get_global_id(0);
      })";
   auto src = ShaderArg<uint16_t>({ 0x3c00, 0x4000, 0x4200, 0x4400,
         auto dest = ShaderArg<float>({ FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX,
         run_shader(kernel_source, 2, 1, 1, src, dest);
   for (unsigned i = 0; i < 8; ++i)
      }
      TEST_F(ComputeTest, vstore_half)
   {
      const char *kernel_source = R"(
   __kernel void main_test(__global half *dst, __global float4 *src)
   {
      int offset = get_global_id(0);
      })";
   auto dest = ShaderArg<uint16_t>({0xdead, 0xdead, 0xdead, 0xdead,
         auto src = ShaderArg<float>({ 1.0, 2.0, 3.0, 4.0,
         run_shader(kernel_source, 2, 1, 1, dest, src);
   const uint16_t expected[] = { 0x3c00, 0x4000, 0x4200, 0x4400,
         for (unsigned i = 0; i < 8; ++i)
      }
      TEST_F(ComputeTest, inline_function)
   {
      const char *kernel_source = R"(
   inline float helper(float foo)
   {
                  __kernel void main_test(__global float *dst, __global float *src)
   {
         })";
   auto dest = ShaderArg<float>({ NAN }, SHADER_ARG_OUTPUT);
   auto src = ShaderArg<float>({ 1.0f }, SHADER_ARG_INPUT);
   run_shader(kernel_source, 1, 1, 1, dest, src);
      }
      TEST_F(ComputeTest, unused_arg)
   {
      const char *kernel_source = R"(
   __kernel void main_test(__global int *dst, __global int *unused, __global int *src)
   {
      int i = get_global_id(0);
      })";
   auto dest = ShaderArg<int>({ -1, -1, -1, -1 }, SHADER_ARG_OUTPUT);
   auto src = ShaderArg<int>({ 1, 2, 3, 4 }, SHADER_ARG_INPUT);
   auto unused = ShaderArg<int>({ -1, -1, -1, -1 }, SHADER_ARG_INPUT);
   run_shader(kernel_source, 4, 1, 1, dest, unused, src);
   for (int i = 0; i < 4; ++i)
      }
      TEST_F(ComputeTest, spec_constant)
   {
      const char *spirv_asm = R"(
               OpCapability Addresses
   OpCapability Kernel
   %1 = OpExtInstImport "OpenCL.std"
         OpMemoryModel Physical64 OpenCL
   %4 = OpString "kernel_arg_type.main_test.uint*,"
         OpSource OpenCL_C 102000
   OpName %__spirv_BuiltInGlobalInvocationId "__spirv_BuiltInGlobalInvocationId"
   OpName %output "output"
   OpName %entry "entry"
   OpName %output_addr "output.addr"
   OpName %id "id"
   OpName %call "call"
   OpName %conv "conv"
   OpName %idxprom "idxprom"
   OpName %arrayidx "arrayidx"
   OpName %add "add"
   OpName %mul "mul"
   OpName %idxprom1 "idxprom1"
   OpName %arrayidx2 "arrayidx2"
   OpDecorate %__spirv_BuiltInGlobalInvocationId BuiltIn GlobalInvocationId
   OpDecorate %__spirv_BuiltInGlobalInvocationId Constant
   OpDecorate %id Alignment 4
      %ulong = OpTypeInt 64 0
      %uint_1 = OpSpecConstant %uint 1
      %_ptr_Input_v3ulong = OpTypePointer Input %v3ulong
         %_ptr_CrossWorkgroup_uint = OpTypePointer CrossWorkgroup %uint
         %_ptr_Function__ptr_CrossWorkgroup_uint = OpTypePointer Function %_ptr_CrossWorkgroup_uint
   %_ptr_Function_uint = OpTypePointer Function %uint
   %__spirv_BuiltInGlobalInvocationId = OpVariable %_ptr_Input_v3ulong Input
            %output = OpFunctionParameter %_ptr_CrossWorkgroup_uint
      %output_addr = OpVariable %_ptr_Function__ptr_CrossWorkgroup_uint Function
            %id = OpVariable %_ptr_Function_uint Function
            %call = OpCompositeExtract %ulong %27 0
   %conv = OpUConvert %uint %call
            %28 = OpLoad %_ptr_CrossWorkgroup_uint %output_addr Aligned 8
   %idxprom = OpUConvert %ulong %29
   %arrayidx = OpInBoundsPtrAccessChain %_ptr_CrossWorkgroup_uint %28 %idxprom
         %30 = OpLoad %uint %arrayidx Aligned 4
      %add = OpIAdd %uint %31 %uint_1
   %mul = OpIMul %uint %30 %add
      %32 = OpLoad %_ptr_CrossWorkgroup_uint %output_addr Aligned 8
      %arrayidx2 = OpInBoundsPtrAccessChain %_ptr_CrossWorkgroup_uint %32 %idxprom1
                  OpStore %arrayidx2 %mul Aligned 4
   Shader shader = assemble(spirv_asm);
            auto inout = ShaderArg<uint32_t>({ 0x00000001, 0x10000001, 0x00020002, 0x04010203 },
         const uint32_t expected[] = {
         };
   CompileArgs args = { (unsigned)inout.size(), 1, 1 };
   run_shader(spec_shader, args, inout);
   for (int i = 0; i < inout.size(); ++i)
      }
      TEST_F(ComputeTest, arg_metadata)
   {
      const char *kernel_source = R"(
   __kernel void main_test(
      __global int *undec_ptr,
   __global volatile int *vol_ptr,
   __global const int *const_ptr,
   __global int *restrict restr_ptr,
   __global const int *restrict const_restr_ptr,
      {
   })";
   Shader shader = compile({ kernel_source });
   EXPECT_EQ(shader.metadata->kernels[0].args[0].address_qualifier, CLC_KERNEL_ARG_ADDRESS_GLOBAL);
   EXPECT_EQ(shader.metadata->kernels[0].args[0].type_qualifier, 0);
   EXPECT_EQ(shader.metadata->kernels[0].args[1].address_qualifier, CLC_KERNEL_ARG_ADDRESS_GLOBAL);
   EXPECT_EQ(shader.metadata->kernels[0].args[1].type_qualifier, CLC_KERNEL_ARG_TYPE_VOLATILE);
   EXPECT_EQ(shader.metadata->kernels[0].args[2].address_qualifier, CLC_KERNEL_ARG_ADDRESS_GLOBAL);
   EXPECT_EQ(shader.metadata->kernels[0].args[2].type_qualifier, CLC_KERNEL_ARG_TYPE_CONST);
   EXPECT_EQ(shader.metadata->kernels[0].args[3].address_qualifier, CLC_KERNEL_ARG_ADDRESS_GLOBAL);
   EXPECT_EQ(shader.metadata->kernels[0].args[3].type_qualifier, CLC_KERNEL_ARG_TYPE_RESTRICT);
   EXPECT_EQ(shader.metadata->kernels[0].args[4].address_qualifier, CLC_KERNEL_ARG_ADDRESS_GLOBAL);
   EXPECT_EQ(shader.metadata->kernels[0].args[4].type_qualifier, CLC_KERNEL_ARG_TYPE_RESTRICT | CLC_KERNEL_ARG_TYPE_CONST);
   EXPECT_EQ(shader.metadata->kernels[0].args[5].address_qualifier, CLC_KERNEL_ARG_ADDRESS_CONSTANT);
      }
