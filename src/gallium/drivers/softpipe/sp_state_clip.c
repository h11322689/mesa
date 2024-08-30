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
   #include "sp_context.h"
   #include "sp_state.h"
   #include "draw/draw_context.h"
         static void
   softpipe_set_clip_state(struct pipe_context *pipe,
         {
               /* pass the clip state to the draw module */
      }
         static void
   softpipe_set_viewport_states(struct pipe_context *pipe,
                     {
               /* pass the viewport info to the draw module */
   draw_set_viewport_states(softpipe->draw, start_slot, num_viewports,
            memcpy(softpipe->viewports + start_slot, viewports,
            }
         static void
   softpipe_set_scissor_states(struct pipe_context *pipe,
                     {
                        assert(start_slot < PIPE_MAX_VIEWPORTS);
            memcpy(softpipe->scissors + start_slot, scissors,
            }
         static void
   softpipe_set_polygon_stipple(struct pipe_context *pipe,
         {
      /* Actual stippling handled by draw module. */
               }
         void
   softpipe_init_clip_funcs(struct pipe_context *pipe)
   {
      pipe->set_clip_state = softpipe_set_clip_state;
   pipe->set_viewport_states = softpipe_set_viewport_states;
   pipe->set_scissor_states = softpipe_set_scissor_states;
      }
