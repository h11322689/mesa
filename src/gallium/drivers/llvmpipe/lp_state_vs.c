   /**************************************************************************
   * 
   * Copyright 2009 VMware, Inc.
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
         #include "pipe/p_defines.h"
   #include "tgsi/tgsi_dump.h"
   #include "util/u_memory.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vs.h"
      #include "lp_context.h"
   #include "lp_debug.h"
   #include "lp_state.h"
         static void *
   llvmpipe_create_vs_state(struct pipe_context *pipe,
         {
               struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
            vs = draw_create_vertex_shader(llvmpipe->draw, templ);
   if (!vs) {
                  if (LP_DEBUG & DEBUG_TGSI && templ->type == PIPE_SHADER_IR_TGSI) {
      debug_printf("llvmpipe: Create vertex shader %p:\n", (void *) vs);
                  }
         static void
   llvmpipe_bind_vs_state(struct pipe_context *pipe, void *_vs)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
            if (llvmpipe->vs == vs)
                                 }
         static void
   llvmpipe_delete_vs_state(struct pipe_context *pipe, void *_vs)
   {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
                        }
            void
   llvmpipe_init_vs_funcs(struct llvmpipe_context *llvmpipe)
   {
      llvmpipe->pipe.create_vs_state = llvmpipe_create_vs_state;
   llvmpipe->pipe.bind_vs_state   = llvmpipe_bind_vs_state;
      }
