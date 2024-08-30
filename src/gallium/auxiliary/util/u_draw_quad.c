   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
         #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_draw_quad.h"
   #include "util/u_memory.h"
   #include "cso_cache/cso_context.h"
         /**
   * Draw a simple vertex buffer / primitive.
   * Limited to float[4] vertex attribs, tightly packed.
   */
   void 
   util_draw_vertex_buffer(struct pipe_context *pipe,
                           struct cso_context *cso,
   struct pipe_resource *vbuf,
   {
                        /* tell pipe about the vertex buffer */
   memset(&vbuffer, 0, sizeof(vbuffer));
   vbuffer.buffer.resource = vbuf;
                     if (cso) {
      cso_set_vertex_buffers(cso, 1, 0, false, &vbuffer);
      } else {
      pipe->set_vertex_buffers(pipe, 1, 0, false, &vbuffer);
         }
         /**
   * Draw a simple vertex buffer / primitive.
   * Limited to float[4] vertex attribs, tightly packed.
   */
   void
   util_draw_user_vertex_buffer(struct cso_context *cso, void *buffer,
               {
                        vbuffer.is_user_buffer = true;
                     cso_set_vertex_buffers(cso, 1, 0, false, &vbuffer);
      }
      /**
   * Draw a user vertex buffer. This is the correct way.
   * util_draw_user_vertex_buffer doesn't work with u_vbuf anymore.
   */
   void
   util_draw_user_vertices(struct cso_context *cso, struct cso_velems_state *ve,
               {
               vbuffer.is_user_buffer = true;
            cso_set_vertex_buffers_and_elements(cso, ve, 1, 0, false, true, &vbuffer);
      }
