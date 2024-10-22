   /**********************************************************
   * Copyright 2008-2022 VMware, Inc.  All rights reserved.
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
   #include "util/u_upload_mgr.h"
      #include "svga_context.h"
   #include "svga_state.h"
   #include "svga_draw.h"
   #include "svga_tgsi.h"
   #include "svga_screen.h"
   #include "svga_shader.h"
   #include "svga_resource_buffer.h"
   #include "svga_hw_reg.h"
            static enum pipe_error
   emit_hw_vs_vdecl(struct svga_context *svga, uint64_t dirty)
   {
      const struct pipe_vertex_element *ve = svga->curr.velems->velem;
   SVGA3dVertexDecl decls[SVGA3D_INPUTREG_MAX];
   unsigned buffer_indexes[SVGA3D_INPUTREG_MAX];
   unsigned i;
            assert(svga->curr.velems->count >=
            /**
   * We can't set the VDECL offset to something negative, so we
   * must calculate a common negative additional index bias, and modify
   * the VDECL offsets accordingly so they *all* end up positive.
   *
   * Note that the exact value of the negative index bias is not that
   * important, since we compensate for it when we calculate the vertex
   * buffer offset below. The important thing is that all vertex buffer
   * offsets remain positive.
   *
   * Note that we use a negative bias variable in order to make the
   * rounding maths more easy to follow, and to avoid int / unsigned
   * confusion.
            for (i = 0; i < svga->curr.velems->count; i++) {
      const struct pipe_vertex_buffer *vb =
         struct svga_buffer *buffer;
   unsigned int offset = vb->buffer_offset + ve[i].src_offset;
            if (!vb->buffer.resource)
            buffer = svga_buffer(vb->buffer.resource);
   if (buffer->uploaded.start > offset) {
      tmp_neg_bias = buffer->uploaded.start - offset;
   if (ve[i].src_stride)
                        for (i = 0; i < svga->curr.velems->count; i++) {
      const struct pipe_vertex_buffer *vb =
         unsigned usage, index;
            if (!vb->buffer.resource)
            buffer = svga_buffer(vb->buffer.resource);
            /* SVGA_NEW_VELEMENT
   */
   decls[i].identity.type = svga->curr.velems->decl_type[i];
   decls[i].identity.method = SVGA3D_DECLMETHOD_DEFAULT;
   decls[i].identity.usage = usage;
   decls[i].identity.usageIndex = index;
            /* Compensate for partially uploaded vbo, and
   * for the negative index bias.
   */
   decls[i].array.offset = (vb->buffer_offset
         + neg_bias * ve[i].src_stride
                                          svga_hwtnl_vertex_decls(svga->hwtnl,
                              svga_hwtnl_vertex_buffers(svga->hwtnl,
                  svga_hwtnl_set_index_bias( svga->hwtnl, -(int) neg_bias );
      }
         static enum pipe_error
   emit_hw_vdecl(struct svga_context *svga, uint64_t dirty)
   {
      /* SVGA_NEW_NEED_SWTNL
   */
   if (svga->state.sw.need_swtnl)
               }
         struct svga_tracked_state svga_hw_vdecl =
   {
      "hw vertex decl state (hwtnl version)",
   ( SVGA_NEW_NEED_SWTNL |
   SVGA_NEW_VELEMENT |
   SVGA_NEW_VBUFFER |
   SVGA_NEW_RAST |
   SVGA_NEW_FS |
   SVGA_NEW_VS ),
      };
