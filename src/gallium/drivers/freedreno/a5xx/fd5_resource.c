   /*
   * Copyright (C) 2018 Rob Clark <robclark@freedesktop.org>
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
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "fd5_resource.h"
      static void
   setup_lrz(struct fd_resource *rsc)
   {
      struct fd_screen *screen = fd_screen(rsc->b.b.screen);
   unsigned lrz_pitch = align(DIV_ROUND_UP(rsc->b.b.width0, 8), 64);
            /* LRZ buffer is super-sampled: */
   switch (rsc->b.b.nr_samples) {
   case 4:
      lrz_pitch *= 2;
      case 2:
                                    rsc->lrz_height = lrz_height;
   rsc->lrz_width = lrz_pitch;
   rsc->lrz_pitch = lrz_pitch;
      }
      uint32_t
   fd5_setup_slices(struct fd_resource *rsc)
   {
               if (FD_DBG(LRZ) && has_depth(prsc->format) && !is_z32(prsc->format))
            fdl5_layout(&rsc->layout, prsc->format, fd_resource_nr_samples(prsc),
                     }
