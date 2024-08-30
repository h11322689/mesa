   /*
   * Copyright 2014, 2015 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "virgl_context.h"
   #include "virgl_encode.h"
   #include "virtio-gpu/virgl_protocol.h"
   #include "virgl_resource.h"
      static struct pipe_stream_output_target *virgl_create_so_target(
      struct pipe_context *ctx,
   struct pipe_resource *buffer,
   unsigned buffer_offset,
      {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_resource *res = virgl_resource(buffer);
   struct virgl_so_target *t = CALLOC_STRUCT(virgl_so_target);
            if (!t)
                  t->base.reference.count = 1;
   t->base.context = ctx;
   pipe_resource_reference(&t->base.buffer, buffer);
   t->base.buffer_offset = buffer_offset;
   t->base.buffer_size = buffer_size;
            res->bind_history |= PIPE_BIND_STREAM_OUTPUT;
   util_range_add(&res->b, &res->valid_buffer_range, buffer_offset,
                  virgl_encoder_create_so_target(vctx, handle, res, buffer_offset, buffer_size);
      }
      static void virgl_destroy_so_target(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
            pipe_resource_reference(&t->base.buffer, NULL);
   virgl_encode_delete_object(vctx, t->handle, VIRGL_OBJECT_STREAMOUT_TARGET);
      }
      static void virgl_set_so_targets(struct pipe_context *ctx,
                     {
      struct virgl_context *vctx = virgl_context(ctx);
   int i;
   for (i = 0; i < num_targets; i++) {
      if (targets[i]) {
                                 } else {
            }
   for (i = num_targets; i < vctx->num_so_targets; i++)
         vctx->num_so_targets = num_targets;
      }
      void virgl_init_so_functions(struct virgl_context *vctx)
   {
      vctx->base.create_stream_output_target = virgl_create_so_target;
   vctx->base.stream_output_target_destroy = virgl_destroy_so_target;
      }
