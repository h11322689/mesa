   /*
   * Copyright (C) 2019 Collabora, Ltd.
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
      #include "util/u_math.h"
   #include "pan_encoder.h"
      /* This file handles attribute descriptors. The
   * bulk of the complexity is from instancing. See mali_job for
   * notes on how this works. But basically, for small vertex
   * counts, we have a lookup table, and for large vertex counts,
   * we look at the high bits as a heuristic. This has to match
   * exactly how the hardware calculates this (which is why the
   * algorithm is so weird) or else instancing will break. */
      /* Given an odd number (of the form 2k + 1), compute k */
   #define ODD(odd) ((odd - 1) >> 1)
      static unsigned
   panfrost_small_padded_vertex_count(unsigned idx)
   {
      if (idx < 10)
         else
      }
      static unsigned
   panfrost_large_padded_vertex_count(uint32_t vertex_count)
   {
      /* First, we have to find the highest set one */
            /* Using that, we mask out the highest 4-bits */
   unsigned n = highest - 4;
            /* Great, we have the nibble. Now we can just try possibilities. Note
   * that we don't care about the bottom most bit in most cases, and we
                     switch (middle_two) {
   case 0b00:
      if (!(nibble & 1))
         else
      case 0b01:
         case 0b10:
         case 0b11:
         default:
            }
      unsigned
   panfrost_padded_vertex_count(unsigned vertex_count)
   {
      if (vertex_count < 20)
         else
      }
      /* The much, much more irritating case -- instancing is enabled. See
   * panfrost_job.h for notes on how this works */
      unsigned
   panfrost_compute_magic_divisor(unsigned hw_divisor, unsigned *o_shift,
         {
      /* We have a NPOT divisor. Here's the fun one (multipling by
            /* floor(log2(d)) */
            /* m = ceil(2^(32 + shift) / d) */
   uint64_t shift_hi = 32 + shift;
   uint64_t t = 1ll << shift_hi;
   double t_f = t;
   double hw_divisor_d = hw_divisor;
   double m_f = ceil(t_f / hw_divisor_d);
            /* Default case */
            /* e = 2^(shift + 32) % d */
            /* Apply round-down algorithm? e <= 2^shift?. XXX: The blob
   * seems to use a different condition */
   if (e <= (1ll << shift)) {
      magic_divisor = m - 1;
               /* Top flag implicitly set */
   assert(magic_divisor & (1u << 31));
   magic_divisor &= ~(1u << 31);
               }
