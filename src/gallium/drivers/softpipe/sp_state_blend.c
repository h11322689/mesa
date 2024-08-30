   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
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
      /* Authors:  Keith Whitwell <keithw@vmware.com>
   */
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "draw/draw_context.h"
   #include "sp_context.h"
   #include "sp_state.h"
         static void *
   softpipe_create_blend_state(struct pipe_context *pipe,
         {
         }
         static void
   softpipe_bind_blend_state(struct pipe_context *pipe,
         {
                                    }
         static void
   softpipe_delete_blend_state(struct pipe_context *pipe,
         {
         }
         static void
   softpipe_set_blend_color(struct pipe_context *pipe,
         {
      struct softpipe_context *softpipe = softpipe_context(pipe);
                              /* save clamped color too */
   for (i = 0; i < 4; i++)
      softpipe->blend_color_clamped.color[i] =
            }
         static void *
   softpipe_create_depth_stencil_state(struct pipe_context *pipe,
         {
         }
         static void
   softpipe_bind_depth_stencil_state(struct pipe_context *pipe,
         {
                           }
         static void
   softpipe_delete_depth_stencil_state(struct pipe_context *pipe, void *depth)
   {
         }
         static void
   softpipe_set_stencil_ref(struct pipe_context *pipe,
         {
                           }
         static void
   softpipe_set_sample_mask(struct pipe_context *pipe,
         {
   }
         void
   softpipe_init_blend_funcs(struct pipe_context *pipe)
   {
      pipe->create_blend_state = softpipe_create_blend_state;
   pipe->bind_blend_state   = softpipe_bind_blend_state;
                     pipe->create_depth_stencil_alpha_state = softpipe_create_depth_stencil_state;
   pipe->bind_depth_stencil_alpha_state   = softpipe_bind_depth_stencil_state;
                        }
