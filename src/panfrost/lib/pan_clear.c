   /*
   * Copyright (C) 2019-2021 Collabora, Ltd.
   * Copyright (C) 2019 Alyssa Rosenzweig
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
   *
   */
      #include "genxml/gen_macros.h"
      #include <string.h>
   #include "util/format_srgb.h"
   #include "util/rounding.h"
   #include "util/u_pack_color.h"
   #include "pan_format.h"
   #include "pan_util.h"
      /* Clear colours are packed as the internal format of the tilebuffer, looked up
   * in the blendable formats table given the render target format.
   *
   * Raw formats may emulate arbitrary formats with blend shaders. For these, we
   * defer to util_pack_colour to pack in the API format.
   *
   * Blendable formats, on the other hand, include extra "fractional" bits in the
   * tilebuffer for dithering. These have a packed fixed-point representation:
   * for a channel with m integer bits and n fractional bits, multiply by ((2^m)
   * - 1) * 2^n and round to the nearest even.
   */
      /* Replicate a 32-bit value to fill 128-bit */
      static void
   pan_pack_color_32(uint32_t *packed, uint32_t v)
   {
      for (unsigned i = 0; i < 4; ++i)
      }
      /* For m integer bits and n fractional bits, calculate the conversion factor,
   * multiply the source value, and convert to integer rounding to even. When
   * dithering, the fractional bits are used. When not dithered, only the integer
   * bits are used and the fractional bits must remain zero. */
      static inline uint32_t
   float_to_fixed(float f, unsigned bits_int, unsigned bits_frac, bool dither)
   {
               if (dither) {
      float factor = m << bits_frac;
      } else {
      uint32_t v = _mesa_roundevenf(f * (float)m);
         }
      struct mali_tib_layout {
      unsigned int_r, frac_r;
   unsigned int_g, frac_g;
   unsigned int_b, frac_b;
      };
      /* clang-format off */
   static const struct mali_tib_layout tib_layouts[] = {
      [MALI_COLOR_BUFFER_INTERNAL_FORMAT_R8G8B8A8]    = {  8, 0,  8, 0,  8, 0, 8, 0 },
   [MALI_COLOR_BUFFER_INTERNAL_FORMAT_R10G10B10A2] = { 10, 0, 10, 0, 10, 0, 2, 0 },
   [MALI_COLOR_BUFFER_INTERNAL_FORMAT_R8G8B8A2]    = {  8, 2,  8, 2,  8, 2, 2, 0 },
   [MALI_COLOR_BUFFER_INTERNAL_FORMAT_R4G4B4A4]    = {  4, 4,  4, 4,  4, 4, 4, 4 },
   [MALI_COLOR_BUFFER_INTERNAL_FORMAT_R5G6B5A0]    = {  5, 5,  6, 4,  5, 5, 0, 2 },
      };
   /* clang-format on */
      /* Raw values are stored as-is but replicated for multisampling */
      static void
   pan_pack_raw(uint32_t *packed, const union pipe_color_union *color,
         {
      union util_color out = {0};
   unsigned size = util_format_get_blocksize(format);
                     if (size == 1) {
      unsigned s = out.ui[0] | (out.ui[0] << 8);
      } else if (size == 2)
         else if (size <= 4)
         else if (size <= 8) {
      memcpy(packed + 0, out.ui, 8);
      } else {
            }
      void
   pan_pack_color(uint32_t *packed, const union pipe_color_union *color,
         {
      /* Set of blendable formats is common across versions. TODO: v9 */
   enum mali_color_buffer_internal_format internal =
            if (internal == MALI_COLOR_BUFFER_INTERNAL_FORMAT_RAW_VALUE) {
      pan_pack_raw(packed, color, format);
               /* Saturate to [0, 1] by definition of UNORM. Prevents overflow. */
   float r = SATURATE(color->f[0]);
   float g = SATURATE(color->f[1]);
   float b = SATURATE(color->f[2]);
            /* Fill in alpha = 1.0 by default */
   if (!util_format_has_alpha(format))
            /* Convert colourspace while we still have floats */
   if (util_format_is_srgb(format)) {
      r = util_format_linear_to_srgb_float(r);
   g = util_format_linear_to_srgb_float(g);
               /* Look up the layout of the tilebuffer */
   assert(internal < ARRAY_SIZE(tib_layouts));
            unsigned count_r = l.int_r + l.frac_r;
   unsigned count_g = l.int_g + l.frac_g + count_r;
   unsigned count_b = l.int_b + l.frac_b + count_g;
            /* Must fill the word */
            /* Convert the transformed float colour to the given layout */
   uint32_t ur = float_to_fixed(r, l.int_r, l.frac_r, dithered) << 0;
   uint32_t ug = float_to_fixed(g, l.int_g, l.frac_g, dithered) << count_r;
   uint32_t ub = float_to_fixed(b, l.int_b, l.frac_b, dithered) << count_g;
               }
