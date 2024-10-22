   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
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
      #include "lp_context.h"
   #include "lp_state.h"
   #include "lp_texture.h"
      #include "util/u_memory.h"
   #include "draw/draw_context.h"
         static struct pipe_stream_output_target *
   llvmpipe_create_so_target(struct pipe_context *pipe,
                     {
      struct draw_so_target *t = CALLOC_STRUCT(draw_so_target);
   if (!t)
            t->target.context = pipe;
   t->target.reference.count = 1;
   pipe_resource_reference(&t->target.buffer, buffer);
   t->target.buffer_offset = buffer_offset;
   t->target.buffer_size = buffer_size;
      }
         static void
   llvmpipe_so_target_destroy(struct pipe_context *pipe,
         {
      pipe_resource_reference(&target->buffer, NULL);
      }
         static uint32_t
   llvmpipe_so_offset(struct pipe_stream_output_target *so_target)
   {
      struct draw_so_target *target = (struct draw_so_target *)so_target;
      }
         static void
   llvmpipe_set_so_targets(struct pipe_context *pipe,
                     {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned i;
   for (i = 0; i < num_targets; i++) {
      const bool append = (offsets[i] == (unsigned)-1);
   /*
   * Warn if the so target was created in another context.
   * XXX Not entirely sure if mesa/st may rely on this?
   * Otherwise should just assert.
   */
   if (targets[i] && targets[i]->context != pipe) {
      debug_printf("Illegal setting of so target with target %d created "
      }
   pipe_so_target_reference((struct pipe_stream_output_target **)
         /* If we're not appending then lets set the internal
         if (!append && llvmpipe->so_targets[i]) {
                  if (targets[i]) {
      void *buf = llvmpipe_resource(targets[i]->buffer)->data;
                  for (; i < llvmpipe->num_so_targets; i++) {
      pipe_so_target_reference((struct pipe_stream_output_target **)
      }
            draw_set_mapped_so_targets(llvmpipe->draw, llvmpipe->num_so_targets,
      }
         void
   llvmpipe_init_so_funcs(struct llvmpipe_context *pipe)
   {
      pipe->pipe.create_stream_output_target = llvmpipe_create_so_target;
   pipe->pipe.stream_output_target_destroy = llvmpipe_so_target_destroy;
   pipe->pipe.set_stream_output_targets = llvmpipe_set_so_targets;
      }
