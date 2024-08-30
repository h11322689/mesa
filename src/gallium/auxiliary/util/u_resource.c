   /*
   * Copyright 2013 VMware, Inc.
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
   * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
         #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_resource.h"
         /**
   * Return the size of the resource in bytes.
   */
   unsigned
   util_resource_size(const struct pipe_resource *res)
   {
      unsigned width = res->width0;
   unsigned height = res->height0;
   unsigned depth = res->depth0;
   unsigned size = 0;
   unsigned level;
            for (level = 0; level <= res->last_level; level++) {
               if (res->target == PIPE_TEXTURE_CUBE)
         else if (res->target == PIPE_TEXTURE_3D)
         else
            size += (util_format_get_nblocksy(res->format, height) *
            width  = u_minify(width, 1);
   height = u_minify(height, 1);
                  }
      /**
   * Return the number of the resources.
   */
   unsigned
   util_resource_num(const struct pipe_resource *res)
   {
      const struct pipe_resource *cur;
            for (count = 0, cur = res; cur; cur = cur->next)
               }
      /**
   * Return the resource at the given index.
   */
   struct pipe_resource *
   util_resource_at_index(const struct pipe_resource *res, unsigned index)
   {
      const struct pipe_resource *cur;
            for (count = 0, cur = res; cur; cur = cur->next) {
      if (count++ == index)
                  }
