   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "util/u_inlines.h"
   #include "pipe/p_defines.h"
   #include "util/u_math.h"
      #include "svga_context.h"
   #include "svga_resource_buffer.h"
         struct svga_constbuf
   {
      unsigned type;
   float (*data)[4];
      };
            static void
   svga_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct svga_screen *svgascreen = svga_screen(pipe->screen);
   struct svga_context *svga = svga_context(pipe);
   struct pipe_resource *buf = cb ? cb->buffer : NULL;
            if (cb) {
               if (cb->user_buffer) {
      buf = svga_user_buffer_create(pipe->screen,
                              assert(shader < PIPE_SHADER_TYPES);
   assert(index < ARRAY_SIZE(svga->curr.constbufs[shader]));
   assert(index < svgascreen->max_const_buffers);
            if (take_ownership) {
      pipe_resource_reference(&svga->curr.constbufs[shader][index].buffer, NULL);
      } else {
                  /* Make sure the constant buffer size to be updated is within the
   * limit supported by the device.
   */
   svga->curr.constbufs[shader][index].buffer_size =
            svga->curr.constbufs[shader][index].buffer_offset = cb ? cb->buffer_offset : 0;
            if (index == 0) {
      if (shader == PIPE_SHADER_FRAGMENT)
         else if (shader == PIPE_SHADER_VERTEX)
         else if (shader == PIPE_SHADER_GEOMETRY)
         else if (shader == PIPE_SHADER_TESS_CTRL)
         else if (shader == PIPE_SHADER_TESS_EVAL)
         else if (shader == PIPE_SHADER_COMPUTE)
      } else {
      if (shader == PIPE_SHADER_FRAGMENT)
         else if (shader == PIPE_SHADER_VERTEX)
         else if (shader == PIPE_SHADER_GEOMETRY)
         else if (shader == PIPE_SHADER_TESS_CTRL)
         else if (shader == PIPE_SHADER_TESS_EVAL)
         else if (shader == PIPE_SHADER_COMPUTE)
            /* update bitmask of dirty const buffers */
            /* purge any stale rawbuf srv */
               if (cb && cb->user_buffer) {
            }
         void
   svga_init_constbuffer_functions(struct svga_context *svga)
   {
         }
   