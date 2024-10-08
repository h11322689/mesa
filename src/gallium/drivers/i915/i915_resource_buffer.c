   /**************************************************************************
   *
   * Copyright 2006 VMware, Inc.
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
   /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Michel Dänzer <daenzer@vmware.com>
   */
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "i915_context.h"
   #include "i915_resource.h"
   #include "i915_screen.h"
      void
   i915_resource_destroy(struct pipe_screen *screen,
         {
      if (resource->target == PIPE_BUFFER) {
      struct i915_buffer *buffer = i915_buffer(resource);
   if (buffer->free_on_destroy)
            } else {
      struct i915_texture *tex = i915_texture(resource);
   struct i915_winsys *iws = i915_screen(screen)->iws;
            if (tex->buffer)
            for (i = 0; i < ARRAY_SIZE(tex->image_offset); i++)
                  }
      void *
   i915_buffer_transfer_map(struct pipe_context *pipe,
                     {
      struct i915_context *i915 = i915_context(pipe);
   struct i915_buffer *buffer = i915_buffer(resource);
            if (!transfer)
            transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;
               }
      void
   i915_buffer_transfer_unmap(struct pipe_context *pipe,
         {
      struct i915_context *i915 = i915_context(pipe);
      }
      void
   i915_buffer_subdata(struct pipe_context *rm_ctx, struct pipe_resource *resource,
               {
                  }
      struct pipe_resource *
   i915_buffer_create(struct pipe_screen *screen,
         {
               if (!buf)
            buf->b = *template;
   pipe_reference_init(&buf->b.reference, 1);
   buf->b.screen = screen;
   buf->data = align_malloc(template->width0, 64);
            if (!buf->data)
                  err:
      FREE(buf);
      }
      struct pipe_resource *
   i915_user_buffer_create(struct pipe_screen *screen, void *ptr, unsigned bytes,
         {
               if (!buf)
            pipe_reference_init(&buf->b.reference, 1);
   buf->b.screen = screen;
   buf->b.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buf->b.usage = PIPE_USAGE_IMMUTABLE;
   buf->b.bind = bind;
   buf->b.flags = 0;
   buf->b.width0 = bytes;
   buf->b.height0 = 1;
   buf->b.depth0 = 1;
            buf->data = ptr;
               }
