   /*
   * Copyright (C) 2018 Jonathan Marek <jonathan@marek.ca>
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
   *    Jonathan Marek <jonathan@marek.ca>
   */
      #include "fd2_resource.h"
      uint32_t
   fd2_setup_slices(struct fd_resource *rsc)
   {
      struct pipe_resource *prsc = &rsc->b.b;
   enum pipe_format format = prsc->format;
   uint32_t height0 = util_format_get_nblocksy(format, prsc->height0);
            /* 32 pixel alignment */
            for (level = 0; level <= prsc->last_level; level++) {
      struct fdl_slice *slice = fd_resource_slice(rsc, level);
   uint32_t pitch = fdl2_pitch(&rsc->layout, level);
            /* mipmaps have power of two sizes in memory */
   if (level)
            slice->offset = size;
                           }
      unsigned
   fd2_tile_mode(const struct pipe_resource *tmpl)
   {
      /* disable tiling for cube maps, freedreno uses a 2D array for the staging
   * texture, (a2xx supports 2D arrays but it is not implemented)
   */
   if (tmpl->target == PIPE_TEXTURE_CUBE)
         /* we can enable tiling for any resource we can render to */
      }
