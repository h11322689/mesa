      '''
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
      /**
   * @file
   * Pixel format packing and unpacking functions.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
   '''
      import sys
      from u_format_parse import *
         def inv_swizzles(swizzles):
      '''Return an array[4] of inverse swizzle terms'''
   '''Only pick the first matching value to avoid l8 getting blue and i8 getting alpha'''
   inv_swizzle = [None]*4
   for i in range(4):
      swizzle = swizzles[i]
   if swizzle < 4 and inv_swizzle[swizzle] == None:
            def print_channels(format, func):
      if format.nr_channels() <= 1:
         else:
      if (format.le_channels == format.be_channels and
         [c.shift for c in format.le_channels] ==
   [c.shift for c in format.be_channels] and
   format.le_swizzles == format.be_swizzles):
   else:
         print('#if UTIL_ARCH_BIG_ENDIAN')
   func(format.be_channels, format.be_swizzles)
   print('#else')
      def generate_format_type(format):
                        def generate_bitfields(channels, swizzles):
      for channel in channels:
         if channel.type == VOID:
      if channel.size:
      elif channel.type == UNSIGNED:
         elif channel.type in (SIGNED, FIXED):
         elif channel.type == FLOAT:
      if channel.size == 64:
         elif channel.size == 32:
         else:
            def generate_full_fields(channels, swizzles):
      for channel in channels:
         assert channel.size % 8 == 0 and is_pot(channel.size)
   if channel.type == VOID:
      if channel.size:
      elif channel.type == UNSIGNED:
         elif channel.type in (SIGNED, FIXED):
         elif channel.type == FLOAT:
      if channel.size == 64:
         elif channel.size == 32:
         elif channel.size == 16:
         else:
            use_bitfields = False
   for channel in format.le_channels:
      if channel.size % 8 or not is_pot(channel.size):
         print('struct util_format_%s {' % format.short_name())
   if use_bitfields:
         else:
         print('};')
            def is_format_supported(format):
      '''Determines whether we actually have the plumbing necessary to generate the
                     if format.layout != PLAIN:
            for i in range(4):
      channel = format.le_channels[i]
   if channel.type not in (VOID, UNSIGNED, SIGNED, FLOAT, FIXED):
         if channel.type == FLOAT and channel.size not in (16, 32, 64):
               def native_type(format):
               if format.name == 'PIPE_FORMAT_R11G11B10_FLOAT':
         if format.name == 'PIPE_FORMAT_R9G9B9E5_FLOAT':
            if format.layout == PLAIN:
      if not format.is_array():
         # For arithmetic pixel formats return the integer type that matches the whole pixel
   else:
         # For array pixel formats return the integer type that matches the color channel
   channel = format.array_element()
   if channel.type in (UNSIGNED, VOID):
         elif channel.type in (SIGNED, FIXED):
         elif channel.type == FLOAT:
      if channel.size == 16:
         elif channel.size == 32:
         elif channel.size == 64:
         else:
         else:
            def intermediate_native_type(bits, sign):
               bytes = 4 # don't use anything smaller than 32bits
   while bytes * 8 < bits:
                  if sign:
         else:
            def get_one_shift(type):
      '''Get the number of the bit that matches unity for this type.'''
   if type.type == 'FLOAT':
         if not type.norm:
         if type.type == UNSIGNED:
         if type.type == SIGNED:
         if type.type == FIXED:
                  def truncate_mantissa(x, bits):
      '''Truncate an integer so it can be represented exactly with a floating
                     s = 1
   if x < 0:
      s = -1
         # We can represent integers up to mantissa + 1 bits exactly
            # Slide the mask until the MSB matches
   shift = 0
   while (x >> shift) & ~mask:
            x &= mask << shift
   x *= s
            def value_to_native(type, value):
      '''Get the value of unity for this type.'''
   if type.type == FLOAT:
      if type.size <= 32 \
         and isinstance(value, int):
      if type.type == FIXED:
         if not type.norm:
         if type.type == UNSIGNED:
         if type.type == SIGNED:
                  def native_to_constant(type, value):
      '''Get the value of unity for this type.'''
   if type.type == FLOAT:
      if type.size <= 32:
         else:
      else:
            def get_one(type):
      '''Get the value of unity for this type.'''
            def clamp_expr(src_channel, dst_channel, dst_native_type, value):
      '''Generate the expression to clamp the value in the source type to the
            if src_channel == dst_channel:
            src_min = src_channel.min()
   src_max = src_channel.max()
   dst_min = dst_channel.min()
            # Translate the destination range to the src native value
   dst_min_native = native_to_constant(src_channel, value_to_native(src_channel, dst_min))
            if src_min < dst_min and src_max > dst_max:
            if src_max > dst_max:
            if src_min < dst_min:
                     def conversion_expr(src_channel,
                     dst_channel, dst_native_type,
   value,
               if src_colorspace != dst_colorspace:
      if src_colorspace == SRGB:
         assert src_channel.type == UNSIGNED
   assert src_channel.norm
   assert src_channel.size <= 8
   assert src_channel.size >= 4
   assert dst_colorspace == RGB
   if src_channel.size < 8:
         if dst_channel.type == FLOAT:
         else:
      assert dst_channel.type == UNSIGNED
   assert dst_channel.norm
      elif dst_colorspace == SRGB:
         assert dst_channel.type == UNSIGNED
   assert dst_channel.norm
   assert dst_channel.size <= 8
   assert src_colorspace == RGB
   if src_channel.type == FLOAT:
         else:
      assert src_channel.type == UNSIGNED
   assert src_channel.norm
   assert src_channel.size == 8
      # XXX rounding is all wrong.
   if dst_channel.size < 8:
         else:
   elif src_colorspace == ZS:
         elif dst_colorspace == ZS:
         else:
         if src_channel == dst_channel:
            src_type = src_channel.type
   src_size = src_channel.size
   src_norm = src_channel.norm
            # Promote half to float
   if src_type == FLOAT and src_size == 16:
      value = '_mesa_half_to_float(%s)' % value
         # Special case for float <-> ubytes for more accurate results
   # Done before clamping since these functions already take care of that
   if src_type == UNSIGNED and src_norm and src_size == 8 and dst_channel.type == FLOAT and dst_channel.size == 32:
         if src_type == FLOAT and src_size == 32 and dst_channel.type == UNSIGNED and dst_channel.norm and dst_channel.size == 8:
            if clamp:
      if dst_channel.type != FLOAT or src_type != FLOAT:
         if src_type in (SIGNED, UNSIGNED) and dst_channel.type in (SIGNED, UNSIGNED):
      if not src_norm and not dst_channel.norm:
                  if src_norm and dst_channel.norm:
         return "_mesa_%snorm_to_%snorm(%s, %d, %d)" % ("s" if src_type == SIGNED else "u",
         else:
         # We need to rescale using an intermediate type big enough to hold the multiplication of both
   src_one = get_one(src_channel)
   dst_one = get_one(dst_channel)
   tmp_native_type = intermediate_native_type(src_size + dst_channel.size, src_channel.sign and dst_channel.sign)
   value = '((%s)%s)' % (tmp_native_type, value)
            # Promote to either float or double
   if src_type != FLOAT:
      if src_norm or src_type == FIXED:
         one = get_one(src_channel)
   if src_size <= 23:
      value = '(%s * (1.0f/0x%x))' % (value, one)
   if dst_channel.size <= 32:
            else:
      # bigger than single precision mantissa, use double
   value = '(%s * (1.0/0x%x))' % (value, one)
      else:
         if src_size <= 23 or dst_channel.size <= 32:
      value = '(float)%s' % value
      else:
      # bigger than single precision mantissa, use double
            # Convert double or float to non-float
   if dst_channel.type != FLOAT:
      if dst_channel.norm or dst_channel.type == FIXED:
         dst_one = get_one(dst_channel)
   if dst_channel.size <= 23:
         else:
            else:
      # Cast double to float when converting to either half or float
   if dst_channel.size <= 32 and src_size > 32:
                  if dst_channel.size == 16:
         elif dst_channel.size == 64 and src_size < 64:
                  def generate_unpack_kernel(format, dst_channel, dst_native_type):
         if not is_format_supported(format):
                     def unpack_from_bitmask(channels, swizzles):
      depth = format.block_size()
   print('         uint%u_t value;' % (depth))
            # Compute the intermediate unshifted values
   for i in range(format.nr_channels()):
         src_channel = channels[i]
   value = 'value'
   shift = src_channel.shift
   if src_channel.type == UNSIGNED:
      if shift:
         if shift + src_channel.size < depth:
            elif src_channel.type == SIGNED:
      if shift + src_channel.size < depth:
      # Align the sign bit
   lshift = depth - (shift + src_channel.size)
      # Cast to signed
   value = '(int%u_t)(%s) ' % (depth, value)
   if src_channel.size < depth:
      # Align the LSB bit
   rshift = depth - src_channel.size
                  # Convert, swizzle, and store final values
   for i in range(4):
         swizzle = swizzles[i]
   if swizzle < 4:
      src_channel = channels[swizzle]
   src_colorspace = format.colorspace
   if src_colorspace == SRGB and i == 3:
      # Alpha channel is linear
      value = src_channel.name
   value = conversion_expr(src_channel,
                  elif swizzle == SWIZZLE_0:
         elif swizzle == SWIZZLE_1:
         elif swizzle == SWIZZLE_NONE:
         else:
         def unpack_from_struct(channels, swizzles):
      print('         struct util_format_%s pixel;' % format.short_name())
            for i in range(4):
         swizzle = swizzles[i]
   if swizzle < 4:
      src_channel = channels[swizzle]
   src_colorspace = format.colorspace
   if src_colorspace == SRGB and i == 3:
      # Alpha channel is linear
      value = 'pixel.%s' % src_channel.name
   value = conversion_expr(src_channel,
                  elif swizzle == SWIZZLE_0:
         elif swizzle == SWIZZLE_1:
         elif swizzle == SWIZZLE_NONE:
         else:
         if format.is_bitmask():
         else:
            def generate_pack_kernel(format, src_channel, src_native_type):
         if not is_format_supported(format):
                              def pack_into_bitmask(channels, swizzles):
               depth = format.block_size()
            for i in range(4):
         dst_channel = channels[i]
   shift = dst_channel.shift
   if inv_swizzle[i] is not None:
      value ='src[%u]' % inv_swizzle[i]
   dst_colorspace = format.colorspace
   if dst_colorspace == SRGB and inv_swizzle[i] == 3:
      # Alpha channel is linear
      value = conversion_expr(src_channel,
                     if dst_channel.type in (UNSIGNED, SIGNED):
      if shift + dst_channel.size < depth:
         if shift:
         if dst_channel.type == SIGNED:
            else:
                     def pack_into_struct(channels, swizzles):
                        for i in range(4):
         dst_channel = channels[i]
   width = dst_channel.size
   if inv_swizzle[i] is None:
         dst_colorspace = format.colorspace
   if dst_colorspace == SRGB and inv_swizzle[i] == 3:
      # Alpha channel is linear
      value ='src[%u]' % inv_swizzle[i]
   value = conversion_expr(src_channel,
                              if format.is_bitmask():
         else:
            def generate_format_unpack(format, dst_channel, dst_native_type, dst_suffix):
                        if "8unorm" in dst_suffix:
         else:
            proto = 'util_format_%s_unpack_%s(%s *restrict dst_row, const uint8_t *restrict src, unsigned width)' % (
                  print('void')
   print(proto)
            if is_format_supported(format):
      print('   %s *dst = dst_row;' % (dst_native_type))
   print(
                     print('      src += %u;' % (format.block_size() / 8,))
   print('      dst += 4;')
         print('}')
            def generate_format_pack(format, src_channel, src_native_type, src_suffix):
                        print('void')
   print('util_format_%s_pack_%s(uint8_t *restrict dst_row, unsigned dst_stride, const %s *restrict src_row, unsigned src_stride, unsigned width, unsigned height)' %
                  print('void util_format_%s_pack_%s(uint8_t *restrict dst_row, unsigned dst_stride, const %s *restrict src_row, unsigned src_stride, unsigned width, unsigned height);' %
            if is_format_supported(format):
      print('   unsigned x, y;')
   print('   for(y = 0; y < height; y += %u) {' % (format.block_height,))
   print('      const %s *src = src_row;' % (src_native_type))
   print('      uint8_t *dst = dst_row;')
                     print('         src += 4;')
   print('         dst += %u;' % (format.block_size() / 8,))
   print('      }')
   print('      dst_row += dst_stride;')
   print('      src_row += src_stride/sizeof(*src_row);')
         print('}')
            def generate_format_fetch(format, dst_channel, dst_native_type):
                        proto = 'util_format_%s_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src, UNUSED unsigned i, UNUSED unsigned j)' % (name)
            print('void')
            print('{')
            if is_format_supported(format):
            print('}')
            def is_format_hand_written(format):
               def generate(formats):
      print()
   print('#include "util/compiler.h"')
   print('#include "util/u_math.h"')
   print('#include "util/half_float.h"')
   print('#include "u_format.h"')
   print('#include "u_format_other.h"')
   print('#include "util/format_srgb.h"')
   print('#include "format_utils.h"')
   print('#include "u_format_yuv.h"')
   print('#include "u_format_zs.h"')
   print('#include "u_format_pack.h"')
            for format in formats:
                                    if format.is_pure_unsigned():
                                                channel = Channel(SIGNED, False, True, 32)
   native_type = 'int'
   suffix = 'signed'
      elif format.is_pure_signed():
                                                native_type = 'unsigned'
   suffix = 'unsigned'
   channel = Channel(UNSIGNED, False, True, 32)
      else:
                                                                     generate_format_unpack(format, channel, native_type, suffix)
   generate_format_pack(format, channel, native_type, suffix)
