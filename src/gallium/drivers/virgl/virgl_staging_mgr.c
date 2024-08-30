   /*
   * Copyright 2009 VMware, Inc.
   * Copyright 2019 Collabora Ltd.
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
      #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "pipe/p_context.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
      #include "virgl_staging_mgr.h"
   #include "virgl_resource.h"
      static bool
   virgl_staging_alloc_buffer(struct virgl_staging_mgr *staging, unsigned min_size)
   {
      struct virgl_winsys *vws = staging->vws;
            /* Release the old buffer, if present:
   */
            /* Allocate a new one:
   */
            staging->hw_res = vws->resource_create(vws,
                                          PIPE_BUFFER,
   NULL,
   PIPE_FORMAT_R8_UNORM,
   VIRGL_BIND_STAGING,
   size,  /* width */
      if (staging->hw_res == NULL)
            staging->map = vws->resource_map(vws, staging->hw_res);
   if (staging->map == NULL) {
      vws->resource_reference(vws, &staging->hw_res, NULL);
               staging->offset = 0;
               }
      void
   virgl_staging_init(struct virgl_staging_mgr *staging, struct pipe_context *pipe,
         {
               staging->vws = virgl_screen(pipe->screen)->vws;
      }
      void
   virgl_staging_destroy(struct virgl_staging_mgr *staging)
   {
      struct virgl_winsys *vws = staging->vws;
      }
      bool
   virgl_staging_alloc(struct virgl_staging_mgr *staging,
                     unsigned size,
   unsigned alignment,
   {
      struct virgl_winsys *vws = staging->vws;
            assert(out_offset);
   assert(outbuf);
   assert(ptr);
            /* Make sure we have enough space in the staging buffer
   * for the sub-allocation.
   */
   if (offset + size > staging->size) {
      if (unlikely(!virgl_staging_alloc_buffer(staging, size))) {
      *out_offset = ~0;
   vws->resource_reference(vws, outbuf, NULL);
   *ptr = NULL;
                           assert(staging->size);
   assert(staging->hw_res);
   assert(staging->map);
   assert(offset < staging->size);
            /* Emit the return values: */
   *ptr = staging->map + offset;
   vws->resource_reference(vws, outbuf, staging->hw_res);
                        }
