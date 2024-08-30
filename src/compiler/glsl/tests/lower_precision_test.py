   # encoding=utf-8
   # Copyright © 2019 Google
      # Permission is hereby granted, free of charge, to any person obtaining a copy
   # of this software and associated documentation files (the "Software"), to deal
   # in the Software without restriction, including without limitation the rights
   # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   # copies of the Software, and to permit persons to whom the Software is
   # furnished to do so, subject to the following conditions:
      # The above copyright notice and this permission notice shall be included in
   # all copies or substantial portions of the Software.
      # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   # SOFTWARE.
      import sys
   import subprocess
   import tempfile
   import re
   from collections import namedtuple
         Test = namedtuple("Test", "name source match_re")
      # NOTE: This test is deprecated, please add any new tests to test_gl_lower_mediump.cpp.
      TESTS = [
      Test("f32 array-of-array with const index",
         """
                                    void main()
   {
         }
   """,
   Test("i32 array-of-array with const index",
         """
   #version 310 es
                                    void main()
   {
         }
   """,
   Test("u32 array-of-array with const index",
         """
   #version 310 es
                                    void main()
   {
         }
   """,
   Test("f32 array-of-array with uniform index",
         """
                                          void main()
   {
         }
   """,
   Test("i32 array-of-array with uniform index",
         """
   #version 310 es
                                          void main()
   {
         }
   """,
   Test("u32 array-of-array with uniform index",
         """
   #version 310 es
                                          void main()
   {
         }
   """,
   Test("f32 function",
                                 mediump float
   get_a()
   {
                  float
   get_b()
   {
                  void main()
   {
         }
   """,
   Test("i32 function",
         """
   #version 310 es
                           mediump int
   get_a()
   {
                  int
   get_b()
   {
                           void main()
   {
         }
   """,
   Test("u32 function",
         """
   #version 310 es
                           mediump uint
   get_a()
   {
                  uint
   get_b()
   {
                           void main()
   {
         }
   """,
   Test("f32 function mediump args",
                                 mediump float
   do_div(float x, float y)
   {
                  void main()
   {
         }
   """,
   Test("i32 function mediump args",
         """
   #version 310 es
                           mediump int
   do_div(int x, int y)
   {
                           void main()
   {
         }
   """,
   Test("u32 function mediump args",
         """
   #version 310 es
                           mediump uint
   do_div(uint x, uint y)
   {
                           void main()
   {
         }
   """,
   Test("f32 function highp args",
                                 mediump float
   do_div(highp float x, highp float y)
   {
                  void main()
   {
         }
   """,
   Test("i32 function highp args",
         """
   #version 310 es
                           mediump int
   do_div(highp int x, highp int y)
   {
                           void main()
   {
         }
   """,
   Test("u32 function highp args",
         """
   #version 310 es
                           mediump uint
   do_div(highp uint x, highp uint y)
   {
                           void main()
   {
         }
            Test("f32 if",
                                 void
   main()
   {
         if (a / b < 0.31)
         else
   }
   """,
   Test("i32 if",
         """
   #version 310 es
                                    void
   main()
   {
         if (a / b < 10)
         else
   }
   """,
   Test("u32 if",
         """
   #version 310 es
                                    void
   main()
   {
         if (a / b < 10u)
         else
   }
   """,
   Test("matrix",
                                       void main()
   {
         }
   """,
   Test("f32 simple struct deref",
                        struct simple {
                           void main()
   {
         }
   """,
   Test("i32 simple struct deref",
         """
   #version 310 es
                  struct simple {
                                    void main()
   {
         }
   """,
   Test("u32 simple struct deref",
         """
   #version 310 es
                  struct simple {
                                    void main()
   {
         }
   """,
   Test("f32 embedded struct deref",
                        struct simple {
                  struct embedded {
                           void main()
   {
         }
   """,
   Test("i32 embedded struct deref",
         """
   #version 310 es
                  struct simple {
                  struct embedded {
                                    void main()
   {
         }
   """,
   Test("u32 embedded struct deref",
         """
   #version 310 es
                  struct simple {
                  struct embedded {
                                    void main()
   {
         }
   """,
   Test("f32 arrayed struct deref",
                        struct simple {
                  struct arrayed {
                           void main()
   {
         }
   """,
   Test("i32 arrayed struct deref",
         """
   #version 310 es
                  struct simple {
                  struct arrayed {
                                    void main()
   {
         }
   """,
   Test("u32 arrayed struct deref",
         """
   #version 310 es
                  struct simple {
                  struct arrayed {
                                    void main()
   {
         }
   """,
   Test("f32 mixed precision not lowered",
         """
                  void main()
   {
         }
   """,
   Test("i32 mixed precision not lowered",
         """
   #version 310 es
                           void main()
   {
         }
   """,
   Test("u32 mixed precision not lowered",
         """
   #version 310 es
                           void main()
   {
         }
   """,
   Test("f32 sampler array",
         """
   #version 320 es
                  uniform sampler2D tex[2];
   // highp shouldn't affect the return value of texture2D
   uniform highp vec2 coord;
                           void main()
   {
         }
   """,
   Test("f32 texture sample",
                        uniform sampler2D tex;
   // highp shouldn't affect the return value of texture2D
                  void main()
   {
         }
   """,
   Test("i32 texture sample",
         """
   #version 310 es
                  uniform mediump isampler2D tex;
   // highp shouldn't affect the return value of texture
                           void main()
   {
         }
   """,
   Test("u32 texture sample",
         """
   #version 310 es
                  uniform mediump usampler2D tex;
   // highp shouldn't affect the return value of texture
                           void main()
   {
         }
   """,
   Test("f32 image array",
         """
                  layout(rgba16f) readonly uniform mediump image2D img[2];
   // highp shouldn't affect the return value of imageLoad
                           void main()
   {
         }
   """,
   Test("f32 image load",
         """
   #version 310 es
                  layout(rgba16f) readonly uniform mediump image2D img;
   // highp shouldn't affect the return value of imageLoad
                           void main()
   {
         }
   """,
   Test("i32 image load",
         """
   #version 310 es
                  layout(rgba16i) readonly uniform mediump iimage2D img;
   // highp shouldn't affect the return value of imageLoad
                           void main()
   {
         }
   """,
   Test("u32 image load",
         """
   #version 310 es
                  layout(rgba16ui) readonly uniform mediump uimage2D img;
   // highp shouldn't affect the return value of imageLoad
                           void main()
   {
         }
   """,
   Test("f32 expression in lvalue",
                        void main()
   {
         gl_FragColor = vec4(1.0);
   }
   """,
   Test("i32 expression in lvalue",
         """
   #version 310 es
                                    void main()
   {
         color = vec4(1.0);
   }
   """,
   Test("f32 builtin with const arg",
                        void main()
   {
         }
   """,
   Test("i32 builtin with const arg",
         """
                           void main()
   {
         }
   """,
   Test("u32 builtin with const arg",
         """
                           void main()
   {
         }
   """,
   Test("dFdx",
         """
                                 void main()
   {
         }
   """,
   Test("dFdy",
         """
                                 void main()
   {
         }
   """,
   Test("textureSize",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("floatBitsToInt",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("floatBitsToUint",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("intBitsToFloat",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("uintBitsToFloat",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("bitfieldReverse",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("frexp",
         """
   #version 310 es
                  uniform float val;
                  void main()
   {
         int y;
   float x = frexp(val + 1.0, y);
   color = x + 1.0;
   }
   """,
   Test("ldexp",
         """
   #version 310 es
                  uniform float val;
                  void main()
   {
         }
   """,
   Test("uaddCarry",
         """
   #version 310 es
                                 void main()
   {
         lowp uint carry;
   color = uaddCarry(x * 2u, y * 2u, carry) * 2u;
   }
   """,
   Test("usubBorrow",
         """
   #version 310 es
                                 void main()
   {
         lowp uint borrow;
   color = usubBorrow(x * 2u, y * 2u, borrow) * 2u;
   }
   """,
   Test("imulExtended",
         """
   #version 310 es
                                 void main()
   {
         int msb, lsb;
   imulExtended(x + 2, y + 2, msb, lsb);
   }
   """,
   Test("umulExtended",
         """
   #version 310 es
                                 void main()
   {
         uint msb, lsb;
   umulExtended(x + 2u, y + 2u, msb, lsb);
   }
   """,
   Test("unpackUnorm2x16",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("unpackSnorm2x16",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("packUnorm2x16",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("packSnorm2x16",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("packHalf2x16",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("packUnorm4x8",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("packSnorm4x8",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("interpolateAtCentroid",
         """
   #version 320 es
                                 void main()
   {
         }
   """,
   Test("interpolateAtOffset",
         """
   #version 320 es
                  uniform highp vec2 offset;
                  void main()
   {
         }
   """,
   Test("interpolateAtSample",
         """
   #version 320 es
                  uniform highp int sample_index;
                  void main()
   {
         }
   """,
   Test("bitfieldExtract",
         """
   #version 310 es
                  uniform highp int offset, bits;
                  void main()
   {
         }
   """,
   Test("bitfieldInsert",
         """
   #version 310 es
                  uniform highp int offset, bits;
                  void main()
   {
         }
   """,
   Test("bitCount",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("findLSB",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("findMSB",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("unpackHalf2x16",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("unpackUnorm4x8",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("unpackSnorm4x8",
         """
   #version 310 es
                                 void main()
   {
         }
   """,
   Test("f32 csel",
         """
                                 void main()
   {
         }
   """,
   Test("i32 csel",
         """
                                 void main()
   {
         }
   """,
   Test("u32 csel",
         """
                                 void main()
   {
         }
   """,
   Test("f32 loop counter",
         """
                                 void main()
   {
         color = 0.0;
   for (float x = 0.0; x < n; x += incr)
   }
   """,
   Test("i32 loop counter",
         """
   #version 310 es
                                 void main()
   {
         color = 0;
   for (int x = 0; x < n; x += incr)
   }
   """,
   Test("u32 loop counter",
         """
   #version 310 es
                                 void main()
   {
         color = 0u;
   for (uint x = 0u; x < n; x += incr)
   }
   """,
   Test("f32 temp array",
         """
                                 void main()
   {
         float a[2] = float[2](x, y);
   if (x > 0.0)
         }
   """,
   Test("i32 temp array",
         """
                                 void main()
   {
         int a[2] = int[2](x, y);
   if (x > 0)
         }
   """,
   Test("u32 temp array",
         """
                                 void main()
   {
         uint a[2] = uint[2](x, y);
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array",
         """
                                 void main()
   {
         float a[2][2] = float[2][2](float[2](x, y), float[2](x, y));
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array",
         """
                                 void main()
   {
         int a[2][2] = int[2][2](int[2](x, y), int[2](x, y));
   if (x > 0)
         }
   """,
   Test("u32 temp array of array",
         """
                                 void main()
   {
         uint a[2][2] = uint[2][2](uint[2](x, y), uint[2](x, y));
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array assigned from highp",
         """
                                 void main()
   {
         highp float b[2][2] = float[2][2](float[2](x, y), float[2](x, y));
   float a[2][2];
   a = b;
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array assigned from highp",
         """
                                 void main()
   {
         highp int b[2][2] = int[2][2](int[2](x, y), int[2](x, y));
   int a[2][2];
   a = b;
   if (x > 0)
         }
   """,
   Test("u32 temp array of array assigned from highp",
         """
                                 void main()
   {
         highp uint b[2][2] = uint[2][2](uint[2](x, y), uint[2](x, y));
   uint a[2][2];
   a = b;
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array assigned to highp",
         """
                                 void main()
   {
         float a[2][2] = float[2][2](float[2](x, y), float[2](x, y));
   highp float b[2][2];
   b = a;
   a = b;
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array assigned to highp",
         """
                                 void main()
   {
         int a[2][2] = int[2][2](int[2](x, y), int[2](x, y));
   highp int b[2][2];
   b = a;
   a = b;
   if (x > 0)
         }
   """,
   Test("u32 temp array of array assigned to highp",
         """
                                 void main()
   {
         uint a[2][2] = uint[2][2](uint[2](x, y), uint[2](x, y));
   highp uint b[2][2];
   b = a;
   a = b;
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array returned by function",
         """
                                 float[2][2] f(void)
   {
                  void main()
   {
         float a[2][2] = f();
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array returned by function",
         """
                                 int[2][2] f(void)
   {
                  void main()
   {
         int a[2][2] = f();
   if (x > 0)
         }
   """,
   Test("u32 temp array of array returned by function",
         """
                                 uint[2][2] f(void)
   {
                  void main()
   {
         uint a[2][2] = f();
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array as function out",
         """
                                 void f(out float[2][2] v)
   {
                  void main()
   {
         float a[2][2];
   f(a);
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array as function out",
         """
                                 void f(out int[2][2] v)
   {
                  void main()
   {
         int a[2][2];
   f(a);
   if (x > 0)
         }
   """,
   Test("u32 temp array of array as function out",
         """
                                 void f(out uint[2][2] v)
   {
                  void main()
   {
         uint a[2][2];
   f(a);
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array as function in",
         """
                                 float[2][2] f(in float[2][2] v)
   {
      float t[2][2] = v;
               void main()
   {
         float a[2][2];
   a = f(a);
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array as function in",
         """
                                 int[2][2] f(in int[2][2] v)
   {
      int t[2][2] = v;
               void main()
   {
         int a[2][2];
   a = f(a);
   if (x > 0)
         }
   """,
   Test("u32 temp array of array as function in",
         """
                                 uint[2][2] f(in uint[2][2] v)
   {
      uint t[2][2] = v;
               void main()
   {
         uint a[2][2];
   a = f(a);
   if (x > 0u)
         }
   """,
   Test("f32 temp array of array as function inout",
         """
                                 void f(inout float[2][2] v)
   {
      float t[2][2] = v;
               void main()
   {
         float a[2][2];
   f(a);
   if (x > 0.0)
         }
   """,
   Test("i32 temp array of array as function inout",
         """
                                 void f(inout int[2][2] v)
   {
      int t[2][2] = v;
               void main()
   {
         int a[2][2];
   f(a);
   if (x > 0)
         }
   """,
   Test("u32 temp array of array as function inout",
         """
                                 void f(inout uint[2][2] v)
   {
      uint t[2][2] = v;
               void main()
   {
         uint a[2][2];
   f(a);
   if (x > 0u)
         }
   """,
   Test("f32 temp struct (not lowered in the presence of control flow - TODO)",
         """
                                 void main()
   {
         struct { float x,y; } s;
   s.x = x;
   s.y = y;
   if (x > 0.0)
         }
   """,
   Test("i32 temp struct (not lowered in the presence of control flow - TODO)",
         """
                                 void main()
   {
         struct { int x,y; } s;
   s.x = x;
   s.y = y;
   if (x > 0)
         }
   """,
   Test("u32 temp struct (not lowered in the presence of control flow - TODO)",
         """
                                 void main()
   {
         struct { uint x,y; } s;
   s.x = x;
   s.y = y;
   if (x > 0u)
         }
            Test("vec4 constructor from float",
         """
                  void main()
   {
         }
            Test("respect copies",
                        void main()
   {
      highp float x = a;
      }
            Test("conversion constructor precision",
         """
   #version 300 es
                  void main()
   {
      /* Constructors don't have a precision qualifier themselves, but
   * constructors are an operation, and so they do the usual "get
   * precision from my operands, or default to the precision of the
   * lvalue" rule.  So, the u2f is done at mediump due to a's precision.
   */
      }
         ]
         def compile_shader(standalone_compiler, source):
      with tempfile.NamedTemporaryFile(mode='wt', suffix='.frag') as source_file:
      print(source, file=source_file)
   source_file.flush()
   return subprocess.check_output([standalone_compiler,
                                 def run_test(standalone_compiler, test):
               if re.search(test.match_re, ir) is None:
      print(ir)
                  def main():
      standalone_compiler = sys.argv[1]
            for test in TESTS:
                        if result:
         print('PASS')
   else:
         print('{}/{} tests returned correct results'.format(passed, len(TESTS)))
            if __name__ == '__main__':
      main()
