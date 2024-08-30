   #
   # Copyright (C) 2014 Connor Abbott
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
   #
   # Authors:
   #    Connor Abbott (cwabbott0@gmail.com)
      import re
      # Class that represents all the information we have about the opcode
   # NOTE: this must be kept in sync with nir_op_info
      class Opcode(object):
      """Class that represents all the information we have about the opcode
   NOTE: this must be kept in sync with nir_op_info
   """
   def __init__(self, name, output_size, output_type, input_sizes,
                           - name is the name of the opcode (prepend nir_op_ for the enum name)
   - all types are strings that get nir_type_ prepended to them
   - input_types is a list of types
   - is_conversion is true if this opcode represents a type conversion
   - algebraic_properties is a space-seperated string, where nir_op_is_ is
   prepended before each entry
   - const_expr is an expression or series of statements that computes the
   constant value of the opcode given the constant values of its inputs.
            Constant expressions are formed from the variables src0, src1, ...,
   src(N-1), where N is the number of arguments.  The output of the
   expression should be stored in the dst variable.  Per-component input
   and output variables will be scalars and non-per-component input and
   output variables will be a struct with fields named x, y, z, and w
   all of the correct type.  Input and output variables can be assumed
   to already be of the correct type and need no conversion.  In
   particular, the conversion from the C bool type to/from  NIR_TRUE and
            For per-component instructions, the entire expression will be
   executed once for each component.  For non-per-component
   instructions, the expression is expected to store the correct values
   in dst.x, dst.y, etc.  If "dst" does not exist anywhere in the
   constant expression, an assignment to dst will happen automatically
   and the result will be equivalent to "dst = <expression>" for
   per-component instructions and "dst.x = dst.y = ... = <expression>"
   for non-per-component instructions.
   """
   assert isinstance(name, str)
   assert isinstance(output_size, int)
   assert isinstance(output_type, str)
   assert isinstance(input_sizes, list)
   assert isinstance(input_sizes[0], int)
   assert isinstance(input_types, list)
   assert isinstance(input_types[0], str)
   assert isinstance(is_conversion, bool)
   assert isinstance(algebraic_properties, str)
   assert isinstance(const_expr, str)
   assert len(input_sizes) == len(input_types)
   assert 0 <= output_size <= 5 or (output_size == 8) or (output_size == 16)
   for size in input_sizes:
      assert 0 <= size <= 5 or (size == 8) or (size == 16)
   if output_size != 0:
      self.name = name
   self.num_inputs = len(input_sizes)
   self.output_size = output_size
   self.output_type = output_type
   self.input_sizes = input_sizes
   self.input_types = input_types
   self.is_conversion = is_conversion
   self.algebraic_properties = algebraic_properties
   self.const_expr = const_expr
      # helper variables for strings
   tfloat = "float"
   tint = "int"
   tbool = "bool"
   tbool1 = "bool1"
   tbool8 = "bool8"
   tbool16 = "bool16"
   tbool32 = "bool32"
   tuint = "uint"
   tuint8 = "uint8"
   tint16 = "int16"
   tuint16 = "uint16"
   tfloat16 = "float16"
   tfloat32 = "float32"
   tint32 = "int32"
   tuint32 = "uint32"
   tint64 = "int64"
   tuint64 = "uint64"
   tfloat64 = "float64"
      _TYPE_SPLIT_RE = re.compile(r'(?P<type>int|uint|float|bool)(?P<bits>\d+)?')
      def type_has_size(type_):
      m = _TYPE_SPLIT_RE.match(type_)
   assert m is not None, 'Invalid NIR type string: "{}"'.format(type_)
         def type_size(type_):
      m = _TYPE_SPLIT_RE.match(type_)
   assert m is not None, 'Invalid NIR type string: "{}"'.format(type_)
   assert m.group('bits') is not None, \
               def type_sizes(type_):
      if type_has_size(type_):
         elif type_ == 'bool':
         elif type_ == 'float':
         else:
         def type_base_type(type_):
      m = _TYPE_SPLIT_RE.match(type_)
   assert m is not None, 'Invalid NIR type string: "{}"'.format(type_)
         # Operation where the first two sources are commutative.
   #
   # For 2-source operations, this just mathematical commutativity.  Some
   # 3-source operations, like ffma, are only commutative in the first two
   # sources.
   _2src_commutative = "2src_commutative "
   associative = "associative "
   selection = "selection "
   derivative = "derivative "
      # global dictionary of opcodes
   opcodes = {}
      def opcode(name, output_size, output_type, input_sizes, input_types,
            assert name not in opcodes
   opcodes[name] = Opcode(name, output_size, output_type, input_sizes,
               def unop_convert(name, out_type, in_type, const_expr, description = ""):
            def unop(name, ty, const_expr, description = "", algebraic_properties = ""):
      opcode(name, 0, ty, [0], [ty], False, algebraic_properties, const_expr,
         def unop_horiz(name, output_size, output_type, input_size, input_type,
            opcode(name, output_size, output_type, [input_size], [input_type],
         def unop_reduce(name, output_size, output_type, input_type, prereduce_expr,
            def prereduce(src):
         def final(src):
         def reduce_(src0, src1):
         src0 = prereduce("src0.x")
   src1 = prereduce("src0.y")
   src2 = prereduce("src0.z")
   src3 = prereduce("src0.w")
   unop_horiz(name + "2", output_size, output_type, 2, input_type,
         unop_horiz(name + "3", output_size, output_type, 3, input_type,
         unop_horiz(name + "4", output_size, output_type, 4, input_type,
               def unop_numeric_convert(name, out_type, in_type, const_expr, description = ""):
            unop("mov", tuint, "src0")
      unop("ineg", tint, "-src0")
   unop("fneg", tfloat, "-src0")
   unop("inot", tint, "~src0", description = "Invert every bit of the integer")
      unop("fsign", tfloat, ("bit_size == 64 ? " +
                     Roughly implements the OpenGL / Vulkan rules for ``sign(float)``.
   The ``GLSL.std.450 FSign`` instruction is defined as:
               If the source is equal to zero, there is a preference for the result to have
   the same sign, but this is not required (it is required by OpenCL).  If the
   source is not a number, there is a preference for the result to be +0.0, but
   this is not required (it is required by OpenCL).  If the source is not a
   number, and the result is not +0.0, the result should definitely **not** be
   NaN.
      The values returned for constant folding match the behavior required by
   OpenCL.
            unop("isign", tint, "(src0 == 0) ? 0 : ((src0 > 0) ? 1 : -1)")
   unop("iabs", tint, "(src0 < 0) ? -src0 : src0")
   unop("fabs", tfloat, "fabs(src0)")
   unop("fsat", tfloat, ("fmin(fmax(src0, 0.0), 1.0)"))
   unop("frcp", tfloat, "bit_size == 64 ? 1.0 / src0 : 1.0f / src0")
   unop("frsq", tfloat, "bit_size == 64 ? 1.0 / sqrt(src0) : 1.0f / sqrtf(src0)")
   unop("fsqrt", tfloat, "bit_size == 64 ? sqrt(src0) : sqrtf(src0)")
   unop("fexp2", tfloat, "exp2f(src0)")
   unop("flog2", tfloat, "log2f(src0)")
      # Generate all of the numeric conversion opcodes
   for src_t in [tint, tuint, tfloat, tbool]:
      if src_t == tbool:
         elif src_t == tint:
         elif src_t == tuint:
         elif src_t == tfloat:
            for dst_t in dst_types:
      for dst_bit_size in type_sizes(dst_t):
      if dst_bit_size == 16 and dst_t == tfloat and src_t == tfloat:
      rnd_modes = ['_rtne', '_rtz', '']
   for rnd_mode in rnd_modes:
         if rnd_mode == '_rtne':
      conv_expr = """
   if (bit_size > 32) {
         } else if (bit_size > 16) {
         } else {
         }
      elif rnd_mode == '_rtz':
      conv_expr = """
   if (bit_size > 32) {
         } else if (bit_size > 16) {
         } else {
         }
      else:
      conv_expr = """
   if (bit_size > 32) {
      if (nir_is_rounding_mode_rtz(execution_mode, 16))
         else
      } else if (bit_size > 16) {
      if (nir_is_rounding_mode_rtz(execution_mode, 16))
         else
                              unop_numeric_convert("{0}2{1}{2}{3}".format(src_t[0],
                        elif dst_bit_size == 32 and dst_t == tfloat and src_t == tfloat:
      conv_expr = """
   if (bit_size > 32 && nir_is_rounding_mode_rtz(execution_mode, 32)) {
         } else {
         }
   """
   unop_numeric_convert("{0}2{1}{2}".format(src_t[0], dst_t[0],
            else:
      conv_expr = "src0 != 0" if dst_t == tbool else "src0"
   unop_numeric_convert("{0}2{1}{2}".format(src_t[0], dst_t[0],
      def unop_numeric_convert_mp(base, src_t, dst_t):
      op_like = base + "16"
   unop_numeric_convert(base + "mp", src_t, dst_t, opcodes[op_like].const_expr,
      Special opcode that is the same as :nir:alu-op:`{}` except that it is safe to
   remove it if the result is immediately converted back to 32 bits again. This is
   generated as part of the precision lowering pass. ``mp`` stands for medium
   precision.
            unop_numeric_convert_mp("f2f", tfloat16, tfloat32)
   unop_numeric_convert_mp("i2i", tint16, tint32)
   # u2ump isn't defined, because the behavior is equal to i2imp
   unop_numeric_convert_mp("f2i", tint16, tfloat32)
   unop_numeric_convert_mp("f2u", tuint16, tfloat32)
   unop_numeric_convert_mp("i2f", tfloat16, tint32)
   unop_numeric_convert_mp("u2f", tfloat16, tuint32)
      # Unary floating-point rounding operations.
         unop("ftrunc", tfloat, "bit_size == 64 ? trunc(src0) : truncf(src0)")
   unop("fceil", tfloat, "bit_size == 64 ? ceil(src0) : ceilf(src0)")
   unop("ffloor", tfloat, "bit_size == 64 ? floor(src0) : floorf(src0)")
   unop("ffract", tfloat, "src0 - (bit_size == 64 ? floor(src0) : floorf(src0))")
   unop("fround_even", tfloat, "bit_size == 64 ? _mesa_roundeven(src0) : _mesa_roundevenf(src0)")
      unop("fquantize2f16", tfloat, "(fabs(src0) < ldexpf(1.0, -14)) ? copysignf(0.0f, src0) : _mesa_half_to_float(_mesa_float_to_half(src0))")
      # Trigonometric operations.
         unop("fsin", tfloat, "bit_size == 64 ? sin(src0) : sinf(src0)")
   unop("fcos", tfloat, "bit_size == 64 ? cos(src0) : cosf(src0)")
      # dfrexp
   unop_convert("frexp_exp", tint32, tfloat, "frexp(src0, &dst);")
   unop_convert("frexp_sig", tfloat, tfloat, "int n; dst = frexp(src0, &n);")
      # Partial derivatives.
   deriv_template = """
   Calculate the screen-space partial derivative using {} derivatives of the input
   with respect to the {}-axis. The constant folding is trivial as the derivative
   of a constant is 0 if the constant is not Inf or NaN.
   """
      for mode, suffix in [("either fine or coarse", ""), ("fine", "_fine"), ("coarse", "_coarse")]:
      for axis in ["x", "y"]:
      unop(f"fdd{axis}{suffix}", tfloat, "isfinite(src0) ? 0.0 : NAN",
            # Floating point pack and unpack operations.
      def pack_2x16(fmt, in_type):
         dst.x = (uint32_t) pack_fmt_1x16(src0.x);
   dst.x |= ((uint32_t) pack_fmt_1x16(src0.y)) << 16;
   """.replace("fmt", fmt))
      def pack_4x8(fmt):
         dst.x = (uint32_t) pack_fmt_1x8(src0.x);
   dst.x |= ((uint32_t) pack_fmt_1x8(src0.y)) << 8;
   dst.x |= ((uint32_t) pack_fmt_1x8(src0.z)) << 16;
   dst.x |= ((uint32_t) pack_fmt_1x8(src0.w)) << 24;
   """.replace("fmt", fmt))
      def unpack_2x16(fmt):
         dst.x = unpack_fmt_1x16((uint16_t)(src0.x & 0xffff));
   dst.y = unpack_fmt_1x16((uint16_t)(src0.x << 16));
   """.replace("fmt", fmt))
      def unpack_4x8(fmt):
         dst.x = unpack_fmt_1x8((uint8_t)(src0.x & 0xff));
   dst.y = unpack_fmt_1x8((uint8_t)((src0.x >> 8) & 0xff));
   dst.z = unpack_fmt_1x8((uint8_t)((src0.x >> 16) & 0xff));
   dst.w = unpack_fmt_1x8((uint8_t)(src0.x >> 24));
   """.replace("fmt", fmt))
         pack_2x16("snorm", tfloat)
   pack_4x8("snorm")
   pack_2x16("unorm", tfloat)
   pack_4x8("unorm")
   pack_2x16("half", tfloat32)
   unpack_2x16("snorm")
   unpack_4x8("snorm")
   unpack_2x16("unorm")
   unpack_4x8("unorm")
   unpack_2x16("half")
      unop_horiz("pack_uint_2x16", 1, tuint32, 2, tuint32, """
   dst.x = _mesa_unsigned_to_unsigned(src0.x, 16);
   dst.x |= _mesa_unsigned_to_unsigned(src0.y, 16) << 16;
   """, description = """
   Convert two unsigned integers into a packed unsigned short (clamp is applied).
   """)
      unop_horiz("pack_sint_2x16", 1, tint32, 2, tint32, """
   dst.x = _mesa_signed_to_signed(src0.x, 16) & 0xffff;
   dst.x |= _mesa_signed_to_signed(src0.y, 16) << 16;
   """, description = """
   Convert two signed integers into a packed signed short (clamp is applied).
   """)
      unop_horiz("pack_uvec2_to_uint", 1, tuint32, 2, tuint32, """
   dst.x = (src0.x & 0xffff) | (src0.y << 16);
   """)
      unop_horiz("pack_uvec4_to_uint", 1, tuint32, 4, tuint32, """
   dst.x = (src0.x <<  0) |
         (src0.y <<  8) |
   (src0.z << 16) |
   """)
      unop_horiz("pack_32_4x8", 1, tuint32, 4, tuint8,
            unop_horiz("pack_32_2x16", 1, tuint32, 2, tuint16,
            unop_horiz("pack_64_2x32", 1, tuint64, 2, tuint32,
            unop_horiz("pack_64_4x16", 1, tuint64, 4, tuint16,
            unop_horiz("unpack_64_2x32", 2, tuint32, 1, tuint64,
            unop_horiz("unpack_64_4x16", 4, tuint16, 1, tuint64,
            unop_horiz("unpack_32_2x16", 2, tuint16, 1, tuint32,
            unop_horiz("unpack_32_4x8", 4, tuint8, 1, tuint32,
            unop_horiz("unpack_half_2x16_flush_to_zero", 2, tfloat32, 1, tuint32, """
   dst.x = unpack_half_1x16_flush_to_zero((uint16_t)(src0.x & 0xffff));
   dst.y = unpack_half_1x16_flush_to_zero((uint16_t)(src0.x << 16));
   """)
      # Lowered floating point unpacking operations.
      unop_convert("unpack_half_2x16_split_x", tfloat32, tuint32,
         unop_convert("unpack_half_2x16_split_y", tfloat32, tuint32,
            unop_convert("unpack_half_2x16_split_x_flush_to_zero", tfloat32, tuint32,
         unop_convert("unpack_half_2x16_split_y_flush_to_zero", tfloat32, tuint32,
            unop_convert("unpack_32_2x16_split_x", tuint16, tuint32, "src0")
   unop_convert("unpack_32_2x16_split_y", tuint16, tuint32, "src0 >> 16")
      unop_convert("unpack_64_2x32_split_x", tuint32, tuint64, "src0")
   unop_convert("unpack_64_2x32_split_y", tuint32, tuint64, "src0 >> 32")
      # Bit operations, part of ARB_gpu_shader5.
         unop("bitfield_reverse", tuint32, """
   /* we're not winning any awards for speed here, but that's ok */
   dst = 0;
   for (unsigned bit = 0; bit < 32; bit++)
         """)
   unop_convert("bit_count", tuint32, tuint, """
   dst = 0;
   for (unsigned bit = 0; bit < bit_size; bit++) {
      if ((src0 >> bit) & 1)
      }
   """)
      unop_convert("ufind_msb", tint32, tuint, """
   dst = -1;
   for (int bit = bit_size - 1; bit >= 0; bit--) {
      if ((src0 >> bit) & 1) {
      dst = bit;
         }
   """)
      unop_convert("ufind_msb_rev", tint32, tuint, """
   dst = -1;
   for (int bit = 0; bit < bit_size; bit++) {
      if ((src0 << bit) & 0x80000000) {
      dst = bit;
         }
   """)
      unop("uclz", tuint32, """
   int bit;
   for (bit = bit_size - 1; bit >= 0; bit--) {
      if ((src0 & (1u << bit)) != 0)
      }
   dst = (unsigned)(bit_size - bit - 1);
   """)
      unop("ifind_msb", tint32, """
   dst = -1;
   for (int bit = bit_size - 1; bit >= 0; bit--) {
      /* If src0 < 0, we're looking for the first 0 bit.
   * if src0 >= 0, we're looking for the first 1 bit.
   */
   if ((((src0 >> bit) & 1) && (src0 >= 0)) ||
      (!((src0 >> bit) & 1) && (src0 < 0))) {
   dst = bit;
         }
   """)
      unop("ifind_msb_rev", tint32, """
   dst = -1;
   /* We are looking for the highest bit that's not the same as the sign bit. */
   uint32_t sign = src0 & 0x80000000u;
   for (int bit = 0; bit < 32; bit++) {
      if (((src0 << bit) & 0x80000000u) != sign) {
      dst = bit;
         }
   """)
      unop_convert("find_lsb", tint32, tint, """
   dst = -1;
   for (unsigned bit = 0; bit < bit_size; bit++) {
      if ((src0 >> bit) & 1) {
      dst = bit;
         }
   """)
      unop_reduce("fsum", 1, tfloat, tfloat, "{src}", "{src0} + {src1}", "{src}",
            def binop_convert(name, out_type, in_type1, alg_props, const_expr, description="", in_type2=None):
      if in_type2 is None:
         opcode(name, 0, out_type, [0, 0], [in_type1, in_type2],
         def binop(name, ty, alg_props, const_expr, description = ""):
            def binop_compare(name, ty, alg_props, const_expr, description = "", ty2=None):
            def binop_compare8(name, ty, alg_props, const_expr, description = "", ty2=None):
            def binop_compare16(name, ty, alg_props, const_expr, description = "", ty2=None):
            def binop_compare32(name, ty, alg_props, const_expr, description = "", ty2=None):
            def binop_compare_all_sizes(name, ty, alg_props, const_expr, description = "", ty2=None):
      binop_compare(name, ty, alg_props, const_expr, description, ty2)
   binop_compare8(name + "8", ty, alg_props, const_expr, description, ty2)
   binop_compare16(name + "16", ty, alg_props, const_expr, description, ty2)
         def binop_horiz(name, out_size, out_type, src1_size, src1_type, src2_size,
            opcode(name, out_size, out_type, [src1_size, src2_size], [src1_type, src2_type],
         def binop_reduce(name, output_size, output_type, src_type, prereduce_expr,
            def final(src):
         def reduce_(src0, src1):
         def prereduce(src0, src1):
         srcs = [prereduce("src0." + letter, "src1." + letter) for letter in "xyzwefghijklmnop"]
   def pairwise_reduce(start, size):
      if (size == 1):
            for size in [2, 4, 8, 16]:
      opcode(name + str(size) + suffix, output_size, output_type,
            opcode(name + "3" + suffix, output_size, output_type,
         [3, 3], [src_type, src_type], False, _2src_commutative,
   opcode(name + "5" + suffix, output_size, output_type,
         [5, 5], [src_type, src_type], False, _2src_commutative,
   final(reduce_(srcs[4], reduce_(reduce_(srcs[3], srcs[2]),
         def binop_reduce_all_sizes(name, output_size, src_type, prereduce_expr,
            binop_reduce(name, output_size, tbool1, src_type,
         binop_reduce("b8" + name[1:], output_size, tbool8, src_type,
         binop_reduce("b16" + name[1:], output_size, tbool16, src_type,
         binop_reduce("b32" + name[1:], output_size, tbool32, src_type,
         binop("fadd", tfloat, _2src_commutative + associative,"""
   if (nir_is_rounding_mode_rtz(execution_mode, bit_size)) {
      if (bit_size == 64)
         else
      } else {
         }
   """)
   binop("iadd", tint, _2src_commutative + associative, "(uint64_t)src0 + (uint64_t)src1")
   binop("iadd_sat", tint, _2src_commutative, """
         src1 > 0 ?
         """)
   binop("uadd_sat", tuint, _2src_commutative,
         binop("isub_sat", tint, "", """
         src1 < 0 ?
         """)
   binop("usub_sat", tuint, "", "src0 < src1 ? 0 : src0 - src1")
      binop("fsub", tfloat, "", """
   if (nir_is_rounding_mode_rtz(execution_mode, bit_size)) {
      if (bit_size == 64)
         else
      } else {
         }
   """)
   binop("isub", tint, "", "src0 - src1")
   binop_convert("uabs_isub", tuint, tint, "", """
               """)
   binop("uabs_usub", tuint, "", "(src1 > src0) ? (src1 - src0) : (src0 - src1)")
      binop("fmul", tfloat, _2src_commutative + associative, """
   if (nir_is_rounding_mode_rtz(execution_mode, bit_size)) {
      if (bit_size == 64)
         else
      } else {
         }
   """)
      binop("fmulz", tfloat32, _2src_commutative + associative, """
   if (src0 == 0.0 || src1 == 0.0)
         else if (nir_is_rounding_mode_rtz(execution_mode, 32))
         else
         """, description = """
   Unlike :nir:alu-op:`fmul`, anything (even infinity or NaN) multiplied by zero is
   always zero. ``fmulz(0.0, inf)`` and ``fmulz(0.0, nan)`` must be +/-0.0, even
   if ``INF_PRESERVE/NAN_PRESERVE`` is not used. If ``SIGNED_ZERO_PRESERVE`` is
   used, then the result must be a positive zero if either operand is zero.
   """)
         binop("imul", tint, _2src_commutative + associative, """
      /* Use 64-bit multiplies to prevent overflow of signed arithmetic */
      """, description = "Low 32-bits of signed/unsigned integer multiply")
      binop_convert("imul_2x32_64", tint64, tint32, _2src_commutative,
               binop_convert("umul_2x32_64", tuint64, tuint32, _2src_commutative,
                  binop("imul_high", tint, _2src_commutative, """
   if (bit_size == 64) {
      /* We need to do a full 128-bit x 128-bit multiply in order for the sign
   * extension to work properly.  The casts are kind-of annoying but needed
   * to prevent compiler warnings.
   */
   uint32_t src0_u32[4] = {
      src0,
   (int64_t)src0 >> 32,
   (int64_t)src0 >> 63,
      };
   uint32_t src1_u32[4] = {
      src1,
   (int64_t)src1 >> 32,
   (int64_t)src1 >> 63,
      };
   uint32_t prod_u32[4];
   ubm_mul_u32arr(prod_u32, src0_u32, src1_u32);
      } else {
      /* First, sign-extend to 64-bit, then convert to unsigned to prevent
   * potential overflow of signed multiply */
      }
   """, description = "High 32-bits of signed integer multiply")
      binop("umul_high", tuint, _2src_commutative, """
   if (bit_size == 64) {
      /* The casts are kind-of annoying but needed to prevent compiler warnings. */
   uint32_t src0_u32[2] = { src0, (uint64_t)src0 >> 32 };
   uint32_t src1_u32[2] = { src1, (uint64_t)src1 >> 32 };
   uint32_t prod_u32[4];
   ubm_mul_u32arr(prod_u32, src0_u32, src1_u32);
      } else {
         }
   """, description = "High 32-bits of unsigned integer multiply")
      binop("umul_low", tuint32, _2src_commutative, """
   uint64_t mask = (1 << (bit_size / 2)) - 1;
   dst = ((uint64_t)src0 & mask) * ((uint64_t)src1 & mask);
   """, description = "Low 32-bits of unsigned integer multiply")
      binop("imul_32x16", tint32, "", "src0 * (int16_t) src1",
         binop("umul_32x16", tuint32, "", "src0 * (uint16_t) src1",
            binop("fdiv", tfloat, "", "src0 / src1")
   binop("idiv", tint, "", "src1 == 0 ? 0 : (src0 / src1)")
   binop("udiv", tuint, "", "src1 == 0 ? 0 : (src0 / src1)")
      binop_convert("uadd_carry", tuint, tuint, _2src_commutative,
               Return an integer (1 or 0) representing the carry resulting from the
   addition of the two unsigned arguments.
            binop_convert("usub_borrow", tuint, tuint, "", "src0 < src1", description = """
   Return an integer (1 or 0) representing the borrow resulting from the
   subtraction of the two unsigned arguments.
            # hadd: (a + b) >> 1 (without overflow)
   # x + y = x - (x & ~y) + (x & ~y) + y - (~x & y) + (~x & y)
   #       =      (x & y) + (x & ~y) +      (x & y) + (~x & y)
   #       = 2 *  (x & y) + (x & ~y) +                (~x & y)
   #       =     ((x & y) << 1) + (x ^ y)
   #
   # Since we know that the bottom bit of (x & y) << 1 is zero,
   #
   # (x + y) >> 1 = (((x & y) << 1) + (x ^ y)) >> 1
   #              =   (x & y) +      ((x ^ y)  >> 1)
   binop("ihadd", tint, _2src_commutative, "(src0 & src1) + ((src0 ^ src1) >> 1)")
   binop("uhadd", tuint, _2src_commutative, "(src0 & src1) + ((src0 ^ src1) >> 1)")
      # rhadd: (a + b + 1) >> 1 (without overflow)
   # x + y + 1 = x + (~x & y) - (~x & y) + y + (x & ~y) - (x & ~y) + 1
   #           =      (x | y) - (~x & y) +      (x | y) - (x & ~y) + 1
   #           = 2 *  (x | y) - ((~x & y) +               (x & ~y)) + 1
   #           =     ((x | y) << 1) - (x ^ y) + 1
   #
   # Since we know that the bottom bit of (x & y) << 1 is zero,
   #
   # (x + y + 1) >> 1 = (x | y) + (-(x ^ y) + 1) >> 1)
   #                  = (x | y) -  ((x ^ y)      >> 1)
   binop("irhadd", tint, _2src_commutative, "(src0 | src1) - ((src0 ^ src1) >> 1)")
   binop("urhadd", tuint, _2src_commutative, "(src0 | src1) - ((src0 ^ src1) >> 1)")
      binop("umod", tuint, "", "src1 == 0 ? 0 : src0 % src1")
      # For signed integers, there are several different possible definitions of
   # "modulus" or "remainder".  We follow the conventions used by LLVM and
   # SPIR-V.  The irem opcode implements the standard C/C++ signed "%"
   # operation while the imod opcode implements the more mathematical
   # "modulus" operation.  For details on the difference, see
   #
   # http://mathforum.org/library/drmath/view/52343.html
      binop("irem", tint, "", "src1 == 0 ? 0 : src0 % src1")
   binop("imod", tint, "",
         "src1 == 0 ? 0 : ((src0 % src1 == 0 || (src0 >= 0) == (src1 >= 0)) ?"
   binop("fmod", tfloat, "", "src0 - src1 * floorf(src0 / src1)")
   binop("frem", tfloat, "", "src0 - src1 * truncf(src0 / src1)")
      #
   # Comparisons
   #
         # these integer-aware comparisons return a boolean (0 or ~0)
      binop_compare_all_sizes("flt", tfloat, "", "src0 < src1")
   binop_compare_all_sizes("fge", tfloat, "", "src0 >= src1")
   binop_compare_all_sizes("feq", tfloat, _2src_commutative, "src0 == src1")
   binop_compare_all_sizes("fneu", tfloat, _2src_commutative, "src0 != src1")
   binop_compare_all_sizes("ilt", tint, "", "src0 < src1")
   binop_compare_all_sizes("ige", tint, "", "src0 >= src1")
   binop_compare_all_sizes("ieq", tint, _2src_commutative, "src0 == src1")
   binop_compare_all_sizes("ine", tint, _2src_commutative, "src0 != src1")
   binop_compare_all_sizes("ult", tuint, "", "src0 < src1")
   binop_compare_all_sizes("uge", tuint, "", "src0 >= src1")
      binop_compare_all_sizes("bitnz", tuint, "", "((uint64_t)src0 >> (src1 & (bit_size - 1)) & 0x1) == 0x1",
            binop_compare_all_sizes("bitz", tuint, "", "((uint64_t)src0 >> (src1 & (bit_size - 1)) & 0x1) == 0x0",
            # integer-aware GLSL-style comparisons that compare floats and ints
      binop_reduce_all_sizes("ball_fequal",  1, tfloat, "{src0} == {src1}",
         binop_reduce_all_sizes("bany_fnequal", 1, tfloat, "{src0} != {src1}",
         binop_reduce_all_sizes("ball_iequal",  1, tint, "{src0} == {src1}",
         binop_reduce_all_sizes("bany_inequal", 1, tint, "{src0} != {src1}",
            # non-integer-aware GLSL-style comparisons that return 0.0 or 1.0
      binop_reduce("fall_equal",  1, tfloat32, tfloat32, "{src0} == {src1}",
         binop_reduce("fany_nequal", 1, tfloat32, tfloat32, "{src0} != {src1}",
            # These comparisons for integer-less hardware return 1.0 and 0.0 for true
   # and false respectively
      binop("slt", tfloat, "", "(src0 < src1) ? 1.0f : 0.0f") # Set on Less Than
   binop("sge", tfloat, "", "(src0 >= src1) ? 1.0f : 0.0f") # Set on Greater or Equal
   binop("seq", tfloat, _2src_commutative, "(src0 == src1) ? 1.0f : 0.0f") # Set on Equal
   binop("sne", tfloat, _2src_commutative, "(src0 != src1) ? 1.0f : 0.0f") # Set on Not Equal
      shift_note = """
   SPIRV shifts are undefined for shift-operands >= bitsize,
   but SM5 shifts are defined to use only the least significant bits.
   The NIR definition is according to the SM5 specification.
   """
      opcode("ishl", 0, tint, [0, 0], [tint, tuint32], False, "",
         "(uint64_t)src0 << (src1 & (sizeof(src0) * 8 - 1))",
   opcode("ishr", 0, tint, [0, 0], [tint, tuint32], False, "",
         "src0 >> (src1 & (sizeof(src0) * 8 - 1))",
   opcode("ushr", 0, tuint, [0, 0], [tuint, tuint32], False, "",
         "src0 >> (src1 & (sizeof(src0) * 8 - 1))",
      opcode("urol", 0, tuint, [0, 0], [tuint, tuint32], False, "", """
      uint32_t rotate_mask = sizeof(src0) * 8 - 1;
   dst = (src0 << (src1 & rotate_mask)) |
      """)
   opcode("uror", 0, tuint, [0, 0], [tuint, tuint32], False, "", """
      uint32_t rotate_mask = sizeof(src0) * 8 - 1;
   dst = (src0 >> (src1 & rotate_mask)) |
      """)
      bitwise_description = """
   Bitwise {0}, also used as a boolean {0} for hardware supporting integers.
   """
      binop("iand", tuint, _2src_commutative + associative, "src0 & src1",
         binop("ior", tuint, _2src_commutative + associative, "src0 | src1",
         binop("ixor", tuint, _2src_commutative + associative, "src0 ^ src1",
               binop_reduce("fdot", 1, tfloat, tfloat, "{src0} * {src1}", "{src0} + {src1}",
            binop_reduce("fdot", 0, tfloat, tfloat,
                  opcode("fdph", 1, tfloat, [3, 4], [tfloat, tfloat], False, "",
         opcode("fdph_replicated", 0, tfloat, [3, 4], [tfloat, tfloat], False, "",
            binop("fmin", tfloat, _2src_commutative + associative, "fmin(src0, src1)")
   binop("imin", tint, _2src_commutative + associative, "src1 > src0 ? src0 : src1")
   binop("umin", tuint, _2src_commutative + associative, "src1 > src0 ? src0 : src1")
   binop("fmax", tfloat, _2src_commutative + associative, "fmax(src0, src1)")
   binop("imax", tint, _2src_commutative + associative, "src1 > src0 ? src1 : src0")
   binop("umax", tuint, _2src_commutative + associative, "src1 > src0 ? src1 : src0")
      binop("fpow", tfloat, "", "bit_size == 64 ? pow(src0, src1) : powf(src0, src1)")
      binop_horiz("pack_half_2x16_split", 1, tuint32, 1, tfloat32, 1, tfloat32,
            binop_horiz("pack_half_2x16_rtz_split", 1, tuint32, 1, tfloat32, 1, tfloat32,
            binop_convert("pack_64_2x32_split", tuint64, tuint32, "",
            binop_convert("pack_32_2x16_split", tuint32, tuint16, "",
            opcode("pack_32_4x8_split", 0, tuint32, [0, 0, 0, 0], [tuint8, tuint8, tuint8, tuint8],
         False, "",
      binop_convert("bfm", tuint32, tint32, "", """
   int bits = src0 & 0x1F;
   int offset = src1 & 0x1F;
   dst = ((1u << bits) - 1) << offset;
   """, description = """
   Implements the behavior of the first operation of the SM5 "bfi" assembly
   and that of the "bfi1" i965 instruction. That is, the bits and offset values
   are from the low five bits of src0 and src1, respectively.
   """)
      opcode("ldexp", 0, tfloat, [0, 0], [tfloat, tint32], False, "", """
   dst = (bit_size == 64) ? ldexp(src0, src1) : ldexpf(src0, src1);
   /* flush denormals to zero. */
   if (!isnormal(dst))
         """)
      binop_horiz("vec2", 2, tuint, 1, tuint, 1, tuint, """
   dst.x = src0.x;
   dst.y = src1.x;
   """, description = """
   Combines the first component of each input to make a 2-component vector.
   """)
      # Byte extraction
   binop("extract_u8", tuint, "", "(uint8_t)(src0 >> (src1 * 8))")
   binop("extract_i8", tint, "", "(int8_t)(src0 >> (src1 * 8))")
      # Word extraction
   binop("extract_u16", tuint, "", "(uint16_t)(src0 >> (src1 * 16))")
   binop("extract_i16", tint, "", "(int16_t)(src0 >> (src1 * 16))")
      # Byte/word insertion
   binop("insert_u8", tuint, "", "(src0 & 0xff) << (src1 * 8)")
   binop("insert_u16", tuint, "", "(src0 & 0xffff) << (src1 * 16)")
         def triop(name, ty, alg_props, const_expr, description = ""):
      opcode(name, 0, ty, [0, 0, 0], [ty, ty, ty], False, alg_props, const_expr,
      def triop_horiz(name, output_size, src1_size, src2_size, src3_size, const_expr,
            opcode(name, output_size, tuint,
   [src1_size, src2_size, src3_size],
         triop("ffma", tfloat, _2src_commutative, """
   if (nir_is_rounding_mode_rtz(execution_mode, bit_size)) {
      if (bit_size == 64)
         else if (bit_size == 32)
         else
      } else {
      if (bit_size == 32)
         else
      }
   """)
      triop("ffmaz", tfloat32, _2src_commutative, """
   if (src0 == 0.0 || src1 == 0.0)
         else if (nir_is_rounding_mode_rtz(execution_mode, 32))
         else
         """, description = """
   Floating-point multiply-add with modified zero handling.
      Unlike :nir:alu-op:`ffma`, anything (even infinity or NaN) multiplied by zero is
   always zero. ``ffmaz(0.0, inf, src2)`` and ``ffmaz(0.0, nan, src2)`` must be
   ``+/-0.0 + src2``, even if ``INF_PRESERVE/NAN_PRESERVE`` is not used. If
   ``SIGNED_ZERO_PRESERVE`` is used, then the result must be a positive
   zero plus src2 if either src0 or src1 is zero.
   """)
      triop("flrp", tfloat, "", "src0 * (1 - src2) + src1 * src2")
      triop("iadd3", tint, _2src_commutative + associative, "src0 + src1 + src2",
            csel_description = """
   A vector conditional select instruction (like ?:, but operating per-
   component on vectors). The condition is {} bool ({}).
   """
      triop("fcsel", tfloat32, selection, "(src0 != 0.0f) ? src1 : src2",
         opcode("bcsel", 0, tuint, [0, 0, 0],
         [tbool1, tuint, tuint], False, selection, "src0 ? src1 : src2",
   opcode("b8csel", 0, tuint, [0, 0, 0],
         [tbool8, tuint, tuint], False, selection, "src0 ? src1 : src2",
   opcode("b16csel", 0, tuint, [0, 0, 0],
         [tbool16, tuint, tuint], False, selection, "src0 ? src1 : src2",
   opcode("b32csel", 0, tuint, [0, 0, 0],
         [tbool32, tuint, tuint], False, selection, "src0 ? src1 : src2",
      triop("i32csel_gt", tint32, selection, "(src0 > 0) ? src1 : src2")
   triop("i32csel_ge", tint32, selection, "(src0 >= 0) ? src1 : src2")
      triop("fcsel_gt", tfloat32, selection, "(src0 > 0.0f) ? src1 : src2")
   triop("fcsel_ge", tfloat32, selection, "(src0 >= 0.0f) ? src1 : src2")
      triop("bfi", tuint32, "", """
   unsigned mask = src0, insert = src1, base = src2;
   if (mask == 0) {
         } else {
      unsigned tmp = mask;
   while (!(tmp & 1)) {
      tmp >>= 1;
      }
      }
   """, description = "SM5 bfi assembly")
         triop("bitfield_select", tuint, "", "(src0 & src1) | (~src0 & src2)")
      # SM5 ubfe/ibfe assembly: only the 5 least significant bits of offset and bits are used.
   opcode("ubfe", 0, tuint32,
         unsigned base = src0;
   unsigned offset = src1 & 0x1F;
   unsigned bits = src2 & 0x1F;
   if (bits == 0) {
         } else if (offset + bits < 32) {
         } else {
         }
   """)
   opcode("ibfe", 0, tint32,
         int base = src0;
   unsigned offset = src1 & 0x1F;
   unsigned bits = src2 & 0x1F;
   if (bits == 0) {
         } else if (offset + bits < 32) {
         } else {
         }
   """)
      # GLSL bitfieldExtract()
   opcode("ubitfield_extract", 0, tuint32,
         unsigned base = src0;
   int offset = src1, bits = src2;
   if (bits == 0) {
         } else if (bits < 0 || offset < 0 || offset + bits > 32) {
         } else {
         }
   """)
   opcode("ibitfield_extract", 0, tint32,
         int base = src0;
   int offset = src1, bits = src2;
   if (bits == 0) {
         } else if (offset < 0 || bits < 0 || offset + bits > 32) {
         } else {
         }
   """)
      # Sum of absolute differences with accumulation.
   # (Equivalent to AMD's v_sad_u8 instruction.)
   # The first two sources contain packed 8-bit unsigned integers, the instruction
   # will calculate the absolute difference of these, and then add them together.
   # There is also a third source which is a 32-bit unsigned integer and added to the result.
   triop_horiz("sad_u8x4", 1, 1, 1, 1, """
   uint8_t s0_b0 = (src0.x & 0x000000ff) >> 0;
   uint8_t s0_b1 = (src0.x & 0x0000ff00) >> 8;
   uint8_t s0_b2 = (src0.x & 0x00ff0000) >> 16;
   uint8_t s0_b3 = (src0.x & 0xff000000) >> 24;
      uint8_t s1_b0 = (src1.x & 0x000000ff) >> 0;
   uint8_t s1_b1 = (src1.x & 0x0000ff00) >> 8;
   uint8_t s1_b2 = (src1.x & 0x00ff0000) >> 16;
   uint8_t s1_b3 = (src1.x & 0xff000000) >> 24;
      dst.x = src2.x +
         (s0_b0 > s1_b0 ? (s0_b0 - s1_b0) : (s1_b0 - s0_b0)) +
   (s0_b1 > s1_b1 ? (s0_b1 - s1_b1) : (s1_b1 - s0_b1)) +
   (s0_b2 > s1_b2 ? (s0_b2 - s1_b2) : (s1_b2 - s0_b2)) +
   """)
      # Combines the first component of each input to make a 3-component vector.
      triop_horiz("vec3", 3, 1, 1, 1, """
   dst.x = src0.x;
   dst.y = src1.x;
   dst.z = src2.x;
   """)
      def quadop_horiz(name, output_size, src1_size, src2_size, src3_size,
            opcode(name, output_size, tuint,
         [src1_size, src2_size, src3_size, src4_size],
         opcode("bitfield_insert", 0, tuint32, [0, 0, 0, 0],
         unsigned base = src0, insert = src1;
   int offset = src2, bits = src3;
   if (bits == 0) {
         } else if (offset < 0 || bits < 0 || bits + offset > 32) {
         } else {
      unsigned mask = ((1ull << bits) - 1) << offset;
      }
   """)
      quadop_horiz("vec4", 4, 1, 1, 1, 1, """
   dst.x = src0.x;
   dst.y = src1.x;
   dst.z = src2.x;
   dst.w = src3.x;
   """)
      opcode("vec5", 5, tuint,
         [1] * 5, [tuint] * 5,
   dst.x = src0.x;
   dst.y = src1.x;
   dst.z = src2.x;
   dst.w = src3.x;
   dst.e = src4.x;
   """)
      opcode("vec8", 8, tuint,
         [1] * 8, [tuint] * 8,
   dst.x = src0.x;
   dst.y = src1.x;
   dst.z = src2.x;
   dst.w = src3.x;
   dst.e = src4.x;
   dst.f = src5.x;
   dst.g = src6.x;
   dst.h = src7.x;
   """)
      opcode("vec16", 16, tuint,
         [1] * 16, [tuint] * 16,
   dst.x = src0.x;
   dst.y = src1.x;
   dst.z = src2.x;
   dst.w = src3.x;
   dst.e = src4.x;
   dst.f = src5.x;
   dst.g = src6.x;
   dst.h = src7.x;
   dst.i = src8.x;
   dst.j = src9.x;
   dst.k = src10.x;
   dst.l = src11.x;
   dst.m = src12.x;
   dst.n = src13.x;
   dst.o = src14.x;
   dst.p = src15.x;
   """)
      # An integer multiply instruction for address calculation.  This is
   # similar to imul, except that the results are undefined in case of
   # overflow.  Overflow is defined according to the size of the variable
   # being dereferenced.
   #
   # This relaxed definition, compared to imul, allows an optimization
   # pass to propagate bounds (ie, from an load/store intrinsic) to the
   # sources, such that lower precision integer multiplies can be used.
   # This is useful on hw that has 24b or perhaps 16b integer multiply
   # instructions.
   binop("amul", tint, _2src_commutative + associative, "src0 * src1")
      # ir3-specific instruction that maps directly to mul-add shift high mix,
   # (IMADSH_MIX16 i.e. ah * bl << 16 + c). It is used for lowering integer
   # multiplication (imul) on Freedreno backend..
   opcode("imadsh_mix16", 0, tint32,
         dst = ((((src0 & 0xffff0000) >> 16) * (src1 & 0x0000ffff)) << 16) + src2;
   """)
      # ir3-specific instruction that maps directly to ir3 mad.s24.
   #
   # 24b multiply into 32b result (with sign extension) plus 32b int
   triop("imad24_ir3", tint32, _2src_commutative,
            # r600/gcn specific instruction that evaluates unnormalized cube texture coordinates
   # and face index
   # The actual texture coordinates are evaluated from this according to
   #    dst.yx / abs(dst.z) + 1.5
   unop_horiz("cube_amd", 4, tfloat32, 3, tfloat32, """
      dst.x = dst.y = dst.z = 0.0;
   float absX = fabsf(src0.x);
   float absY = fabsf(src0.y);
            if (absX >= absY && absX >= absZ) { dst.z = 2 * src0.x; }
   if (absY >= absX && absY >= absZ) { dst.z = 2 * src0.y; }
            if (src0.x >= 0 && absX >= absY && absX >= absZ) {
         }
   if (src0.x < 0 && absX >= absY && absX >= absZ) {
         }
   if (src0.y >= 0 && absY >= absX && absY >= absZ) {
         }
   if (src0.y < 0 && absY >= absX && absY >= absZ) {
         }
   if (src0.z >= 0 && absZ >= absX && absZ >= absY) {
         }
   if (src0.z < 0 && absZ >= absX && absZ >= absY) {
            """)
      # r600/gcn specific sin and cos
   # these trigeometric functions need some lowering because the supported
   # input values are expected to be normalized by dividing by (2 * pi)
   unop("fsin_amd", tfloat, "sinf(6.2831853 * src0)")
   unop("fcos_amd", tfloat, "cosf(6.2831853 * src0)")
      # Midgard specific sin and cos
   # These expect their inputs to be divided by pi.
   unop("fsin_mdg", tfloat, "sinf(3.141592653589793 * src0)")
   unop("fcos_mdg", tfloat, "cosf(3.141592653589793 * src0)")
      # AGX specific sin with input expressed in quadrants. Used in the lowering for
   # fsin/fcos. This corresponds to a sequence of 3 ALU ops in the backend (where
   # the angle is further decomposed by quadrant, sinc is computed, and the angle
   # is multiplied back for sin). Lowering fsin/fcos to fsin_agx requires some
   # additional ALU that NIR may be able to optimize.
   unop("fsin_agx", tfloat, "sinf(src0 * (6.2831853/4.0))")
      # AGX specific bitfield extraction from a pair of 32bit registers.
   # src0,src1: the two registers
   # src2: bit position of the LSB of the bitfield
   # src3: number of bits in the bitfield if src3 > 0
   #       src3 = 0 is equivalent to src3 = 32
   # NOTE: src3 is a nir constant by contract
   opcode("extr_agx", 0, tuint32,
            uint32_t mask = 0xFFFFFFFF;
   uint8_t shift = src2 & 0x7F;
   if (src3 != 0) {
         }
   if (shift >= 64) {
         } else {
            """);
      # AGX multiply-shift-add. Corresponds to iadd/isub/imad/imsub instructions.
   # The shift must be <= 4 (domain restriction). For performance, it should be
   # constant.
   opcode("imadshl_agx", 0, tint, [0, 0, 0, 0], [tint, tint, tint, tint], False,
         opcode("imsubshl_agx", 0, tint, [0, 0, 0, 0], [tint, tint, tint, tint], False,
            binop_convert("interleave_agx", tuint32, tuint16, "", """
         dst = 0;
   for (unsigned bit = 0; bit < 16; bit++) {
      dst |= (src0 & (1 << bit)) << bit;
      }""", description="""
   Interleave bits of 16-bit integers to calculate a 32-bit integer. This can
   be used as-is for Morton encoding.
      # 24b multiply into 32b result (with sign extension)
   binop("imul24", tint32, _2src_commutative + associative,
            # unsigned 24b multiply into 32b result plus 32b int
   triop("umad24", tuint32, _2src_commutative,
            # unsigned 24b multiply into 32b result uint
   binop("umul24", tint32, _2src_commutative + associative,
            # relaxed versions of the above, which assume input is in the 24bit range (no clamping)
   binop("imul24_relaxed", tint32, _2src_commutative + associative, "src0 * src1")
   triop("umad24_relaxed", tuint32, _2src_commutative, "src0 * src1 + src2")
   binop("umul24_relaxed", tuint32, _2src_commutative + associative, "src0 * src1")
      unop_convert("fisnormal", tbool1, tfloat, "isnormal(src0)")
   unop_convert("fisfinite", tbool1, tfloat, "isfinite(src0)")
   unop_convert("fisfinite32", tbool32, tfloat, "isfinite(src0)")
      # vc4-specific opcodes
      # Saturated vector add for 4 8bit ints.
   binop("usadd_4x8_vc4", tint32, _2src_commutative + associative, """
   dst = 0;
   for (int i = 0; i < 32; i += 8) {
         }
   """)
      # Saturated vector subtract for 4 8bit ints.
   binop("ussub_4x8_vc4", tint32, "", """
   dst = 0;
   for (int i = 0; i < 32; i += 8) {
      int src0_chan = (src0 >> i) & 0xff;
   int src1_chan = (src1 >> i) & 0xff;
   if (src0_chan > src1_chan)
      }
   """)
      # vector min for 4 8bit ints.
   binop("umin_4x8_vc4", tint32, _2src_commutative + associative, """
   dst = 0;
   for (int i = 0; i < 32; i += 8) {
         }
   """)
      # vector max for 4 8bit ints.
   binop("umax_4x8_vc4", tint32, _2src_commutative + associative, """
   dst = 0;
   for (int i = 0; i < 32; i += 8) {
         }
   """)
      # unorm multiply: (a * b) / 255.
   binop("umul_unorm_4x8_vc4", tint32, _2src_commutative + associative, """
   dst = 0;
   for (int i = 0; i < 32; i += 8) {
      int src0_chan = (src0 >> i) & 0xff;
   int src1_chan = (src1 >> i) & 0xff;
      }
   """)
      # Mali-specific opcodes
   unop("fsat_signed_mali", tfloat, ("fmin(fmax(src0, -1.0), 1.0)"))
   unop("fclamp_pos_mali", tfloat, ("fmax(src0, 0.0)"))
      opcode("b32fcsel_mdg", 0, tuint, [0, 0, 0],
         [tbool32, tfloat, tfloat], False, selection, "src0 ? src1 : src2",
   description = csel_description.format("a 32-bit", "0 vs ~0") + """
   This Midgard-specific variant takes floating-point sources, rather than
   integer sources. That includes support for floating point modifiers in
   the backend.
      # Magnitude equal to fddx/y, sign undefined. Derivative of a constant is zero.
   unop("fddx_must_abs_mali", tfloat, "0.0", algebraic_properties = "derivative")
   unop("fddy_must_abs_mali", tfloat, "0.0", algebraic_properties = "derivative")
      # DXIL specific double [un]pack
   # DXIL doesn't support generic [un]pack instructions, so we want those
   # lowered to bit ops. HLSL doesn't support 64bit bitcasts to/from
   # double, only [un]pack. Technically DXIL does, but considering they
   # can't be generated from HLSL, we want to match what would be coming from DXC.
   # This is essentially just the standard [un]pack, except that it doesn't get
   # lowered so we can handle it in the backend and turn it into MakeDouble/SplitDouble
   unop_horiz("pack_double_2x32_dxil", 1, tuint64, 2, tuint32,
         unop_horiz("unpack_double_2x32_dxil", 2, tuint32, 1, tuint64,
            # src0 and src1 are i8vec4 packed in an int32, and src2 is an int32.  The int8
   # components are sign-extended to 32-bits, and a dot-product is performed on
   # the resulting vectors.  src2 is added to the result of the dot-product.
   opcode("sdot_4x8_iadd", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const int32_t v0x = (int8_t)(src0      );
   const int32_t v0y = (int8_t)(src0 >>  8);
   const int32_t v0z = (int8_t)(src0 >> 16);
   const int32_t v0w = (int8_t)(src0 >> 24);
   const int32_t v1x = (int8_t)(src1      );
   const int32_t v1y = (int8_t)(src1 >>  8);
   const int32_t v1z = (int8_t)(src1 >> 16);
               """)
      # Like sdot_4x8_iadd, but unsigned.
   opcode("udot_4x8_uadd", 0, tuint32, [0, 0, 0], [tuint32, tuint32, tuint32],
            const uint32_t v0x = (uint8_t)(src0      );
   const uint32_t v0y = (uint8_t)(src0 >>  8);
   const uint32_t v0z = (uint8_t)(src0 >> 16);
   const uint32_t v0w = (uint8_t)(src0 >> 24);
   const uint32_t v1x = (uint8_t)(src1      );
   const uint32_t v1y = (uint8_t)(src1 >>  8);
   const uint32_t v1z = (uint8_t)(src1 >> 16);
               """)
      # src0 is i8vec4 packed in an int32, src1 is u8vec4 packed in an int32, and
   # src2 is an int32.  The 8-bit components are extended to 32-bits, and a
   # dot-product is performed on the resulting vectors.  src2 is added to the
   # result of the dot-product.
   #
   # NOTE: Unlike many of the other dp4a opcodes, this mixed signs of source 0
   # and source 1 mean that this opcode is not 2-source commutative
   opcode("sudot_4x8_iadd", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const int32_t v0x = (int8_t)(src0      );
   const int32_t v0y = (int8_t)(src0 >>  8);
   const int32_t v0z = (int8_t)(src0 >> 16);
   const int32_t v0w = (int8_t)(src0 >> 24);
   const uint32_t v1x = (uint8_t)(src1      );
   const uint32_t v1y = (uint8_t)(src1 >>  8);
   const uint32_t v1z = (uint8_t)(src1 >> 16);
               """)
      # Like sdot_4x8_iadd, but the result is clampled to the range [-0x80000000, 0x7ffffffff].
   opcode("sdot_4x8_iadd_sat", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const int64_t v0x = (int8_t)(src0      );
   const int64_t v0y = (int8_t)(src0 >>  8);
   const int64_t v0z = (int8_t)(src0 >> 16);
   const int64_t v0w = (int8_t)(src0 >> 24);
   const int64_t v1x = (int8_t)(src1      );
   const int64_t v1y = (int8_t)(src1 >>  8);
   const int64_t v1z = (int8_t)(src1 >> 16);
                        """)
      # Like udot_4x8_uadd, but the result is clampled to the range [0, 0xfffffffff].
   opcode("udot_4x8_uadd_sat", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const uint64_t v0x = (uint8_t)(src0      );
   const uint64_t v0y = (uint8_t)(src0 >>  8);
   const uint64_t v0z = (uint8_t)(src0 >> 16);
   const uint64_t v0w = (uint8_t)(src0 >> 24);
   const uint64_t v1x = (uint8_t)(src1      );
   const uint64_t v1y = (uint8_t)(src1 >>  8);
   const uint64_t v1z = (uint8_t)(src1 >> 16);
                        """)
      # Like sudot_4x8_iadd, but the result is clampled to the range [-0x80000000, 0x7ffffffff].
   #
   # NOTE: Unlike many of the other dp4a opcodes, this mixed signs of source 0
   # and source 1 mean that this opcode is not 2-source commutative
   opcode("sudot_4x8_iadd_sat", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const int64_t v0x = (int8_t)(src0      );
   const int64_t v0y = (int8_t)(src0 >>  8);
   const int64_t v0z = (int8_t)(src0 >> 16);
   const int64_t v0w = (int8_t)(src0 >> 24);
   const uint64_t v1x = (uint8_t)(src1      );
   const uint64_t v1y = (uint8_t)(src1 >>  8);
   const uint64_t v1z = (uint8_t)(src1 >> 16);
                        """)
      # src0 and src1 are i16vec2 packed in an int32, and src2 is an int32.  The int16
   # components are sign-extended to 32-bits, and a dot-product is performed on
   # the resulting vectors.  src2 is added to the result of the dot-product.
   opcode("sdot_2x16_iadd", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const int32_t v0x = (int16_t)(src0      );
   const int32_t v0y = (int16_t)(src0 >> 16);
   const int32_t v1x = (int16_t)(src1      );
               """)
      # Like sdot_2x16_iadd, but unsigned.
   opcode("udot_2x16_uadd", 0, tuint32, [0, 0, 0], [tuint32, tuint32, tuint32],
            const uint32_t v0x = (uint16_t)(src0      );
   const uint32_t v0y = (uint16_t)(src0 >> 16);
   const uint32_t v1x = (uint16_t)(src1      );
               """)
      # Like sdot_2x16_iadd, but the result is clampled to the range [-0x80000000, 0x7ffffffff].
   opcode("sdot_2x16_iadd_sat", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const int64_t v0x = (int16_t)(src0      );
   const int64_t v0y = (int16_t)(src0 >> 16);
   const int64_t v1x = (int16_t)(src1      );
                        """)
      # Like udot_2x16_uadd, but the result is clampled to the range [0, 0xfffffffff].
   opcode("udot_2x16_uadd_sat", 0, tint32, [0, 0, 0], [tuint32, tuint32, tint32],
            const uint64_t v0x = (uint16_t)(src0      );
   const uint64_t v0y = (uint16_t)(src0 >> 16);
   const uint64_t v1x = (uint16_t)(src1      );
                        """)
