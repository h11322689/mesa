      '''
   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
   '''
         import copy
      VOID, UNSIGNED, SIGNED, FIXED, FLOAT = range(5)
      SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W, SWIZZLE_0, SWIZZLE_1, SWIZZLE_NONE, = range(7)
      PLAIN = 'plain'
      RGB = 'rgb'
   SRGB = 'srgb'
   YUV = 'yuv'
   ZS = 'zs'
         def is_pot(x):
               VERY_LARGE = 99999999999999999999999
         class Channel:
               def __init__(self, type, norm, pure, size, name=''):
      self.type = type
   self.norm = norm
   self.pure = pure
   self.size = size
   self.sign = type in (SIGNED, FIXED, FLOAT)
         def __str__(self):
      s = str(self.type)
   if self.norm:
         if self.pure:
         s += str(self.size)
         def __repr__(self):
            def __eq__(self, other):
      if other is None:
                  def __ne__(self, other):
            def max(self):
      '''Maximum representable number.'''
   if self.type == FLOAT:
         if self.type == FIXED:
         if self.norm:
         if self.type == UNSIGNED:
         if self.type == SIGNED:
               def min(self):
      '''Minimum representable number.'''
   if self.type == FLOAT:
         if self.type == FIXED:
         if self.type == UNSIGNED:
         if self.norm:
         if self.type == SIGNED:
               class Format:
               def __init__(self, name, layout, block_width, block_height, block_depth, le_channels, le_swizzles, be_channels, be_swizzles, colorspace):
      self.name = name
   self.layout = layout
   self.block_width = block_width
   self.block_height = block_height
   self.block_depth = block_depth
            self.le_channels = le_channels
            le_shift = 0
   for channel in self.le_channels:
                  if be_channels:
         if self.is_array():
      print(
            if self.is_bitmask():
      print(
            self.be_channels = be_channels
   elif self.is_bitmask() and not self.is_array():
         # Bitmask formats are "load a word the size of the block and
   # bitshift channels out of it." However, the channel shifts
   # defined in u_format_table.c are numbered right-to-left on BE
   # for some historical reason (see below), which is hard to
   # change due to llvmpipe, so we also have to flip the channel
   # order and the channel-to-rgba swizzle values to read
   # right-to-left from the defined (non-VOID) channels so that the
   # correct shifts happen.
   #
   # This is nonsense, but it's the nonsense that makes
   # u_format_test pass and you get the right colors in softpipe at
   # least.
   chans = self.nr_channels()
                  xyzw = [SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W]
   chan_map = {SWIZZLE_X: xyzw[chans - 1] if chans >= 1 else SWIZZLE_X,
               SWIZZLE_Y: xyzw[chans - 2] if chans >= 2 else SWIZZLE_X,
   SWIZZLE_Z: xyzw[chans - 3] if chans >= 3 else SWIZZLE_X,
   SWIZZLE_W: xyzw[chans - 4] if chans >= 4 else SWIZZLE_X,
   SWIZZLE_1: SWIZZLE_1,
   else:
                  be_shift = 0
   for channel in reversed(self.be_channels):
                  assert le_shift == be_shift
   for i in range(4):
               def __str__(self):
            def short_name(self):
      '''Make up a short norm for a format, suitable to be used as suffix in
            name = self.name
   if name.startswith('PIPE_FORMAT_'):
         name = name.lower()
         def block_size(self):
      size = 0
   for channel in self.le_channels:
               def nr_channels(self):
      nr_channels = 0
   for channel in self.le_channels:
         if channel.size:
         def array_element(self):
      if self.layout != PLAIN:
         ref_channel = self.le_channels[0]
   if ref_channel.type == VOID:
         for channel in self.le_channels:
         if channel.size and (channel.size != ref_channel.size or channel.size % 8):
         if channel.type != VOID:
      if channel.type != ref_channel.type:
         if channel.norm != ref_channel.norm:
                  def is_array(self):
            def is_mixed(self):
      if self.layout != PLAIN:
         ref_channel = self.le_channels[0]
   if ref_channel.type == VOID:
         for channel in self.le_channels[1:]:
         if channel.type != VOID:
      if channel.type != ref_channel.type:
         if channel.norm != ref_channel.norm:
                  def is_compressed(self):
      for channel in self.le_channels:
         if channel.type != VOID:
         def is_unorm(self):
      # Non-compressed formats all have unorm or srgb in their name.
   for keyword in ['_UNORM', '_SRGB']:
                  # All the compressed formats in GLES3.2 and GL4.6 ("Table 8.14: Generic
   # and specific compressed internal formats.") that aren't snorm for
   # border colors are unorm, other than BPTC_*_FLOAT.
         def is_snorm(self):
            def is_pot(self):
            def is_int(self):
      if self.layout != PLAIN:
         for channel in self.le_channels:
         if channel.type not in (VOID, UNSIGNED, SIGNED):
         def is_float(self):
      if self.layout != PLAIN:
         for channel in self.le_channels:
         if channel.type not in (VOID, FLOAT):
         def is_bitmask(self):
      if self.layout != PLAIN:
         if self.block_size() not in (8, 16, 32):
         for channel in self.le_channels:
         if channel.type not in (VOID, UNSIGNED, SIGNED):
         def is_pure_color(self):
      if self.layout != PLAIN or self.colorspace == ZS:
         pures = [channel.pure
               for x in pures:
               def channel_type(self):
      types = [channel.type
               for x in types:
               def is_pure_signed(self):
            def is_pure_unsigned(self):
            def has_channel(self, id):
            def has_depth(self):
            def has_stencil(self):
            def stride(self):
            _type_parse_map = {
      '':  VOID,
   'x': VOID,
   'u': UNSIGNED,
   's': SIGNED,
   'h': FIXED,
      }
      _swizzle_parse_map = {
      'x': SWIZZLE_X,
   'y': SWIZZLE_Y,
   'z': SWIZZLE_Z,
   'w': SWIZZLE_W,
   '0': SWIZZLE_0,
   '1': SWIZZLE_1,
      }
         def _parse_channels(fields, layout, colorspace, swizzles):
      if layout == PLAIN:
      names = ['']*4
   if colorspace in (RGB, SRGB):
         for i in range(4):
      swizzle = swizzles[i]
      elif colorspace == ZS:
         for i in range(4):
      swizzle = swizzles[i]
      else:
         for i in range(4):
            else:
            channels = []
   for i in range(0, 4):
      field = fields[i]
   if field:
         type = _type_parse_map[field[0]]
   if field[1] == 'n':
      norm = True
   pure = False
      elif field[1] == 'p':
      pure = True
   norm = False
      else:
      norm = False
      else:
         type = VOID
   norm = False
   pure = False
   channel = Channel(type, norm, pure, size, names[i])
                  def parse(filename):
      '''Parse the format description in CSV format in terms of the
            stream = open(filename)
   formats = []
   for line in stream:
      try:
         except ValueError:
         else:
         line = line.strip()
   if not line:
            fields = [field.strip() for field in line.split(',')]
            name = fields[0]
   layout = fields[1]
   block_width, block_height, block_depth = map(int, fields[2:5])
            le_swizzles = [_swizzle_parse_map[swizzle] for swizzle in fields[9]]
            be_swizzles = None
   be_channels = None
   if len(fields) == 16:
                                    format = Format(name, layout, block_width, block_height, block_depth, le_channels, le_swizzles, be_channels, be_swizzles, colorspace)
      return formats
