   /**********************************************************
   * Copyright 2022 VMware, Inc.  All rights reserved.
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
      #include "pipe/p_defines.h"
   #include "util/u_bitmask.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_context.h"
   #include "svga_cmd.h"
   #include "svga_debug.h"
   #include "svga_resource_buffer.h"
   #include "svga_resource_texture.h"
   #include "svga_surface.h"
   #include "svga_sampler_view.h"
   #include "svga_format.h"
         /**
   * Create a uav object for the specified shader buffer
   */
   SVGA3dUAViewId
   svga_create_uav_buffer(struct svga_context *svga,
                     {
      SVGA3dUAViewDesc desc;
                     /* If there is not one defined, create one. */
   memset(&desc, 0, sizeof(desc));
   desc.buffer.firstElement = buf->buffer_offset / sizeof(uint32);
   desc.buffer.numElements = buf->buffer_size / sizeof(uint32);
            uaViewId = svga_create_uav(svga, &desc, format,
                     if (uaViewId == SVGA3D_INVALID_ID)
            SVGA_DBG(DEBUG_UAV, "%s: resource=0x%x uaViewId=%d\n",
            /* Mark this buffer as a uav bound buffer */
   struct svga_buffer *sbuf = svga_buffer(buf->buffer);
               }
         /**
   * Set shader buffers.
   */
   static void
   svga_set_shader_buffers(struct pipe_context *pipe,
                           {
      struct svga_context *svga = svga_context(pipe);
                           #ifdef DEBUG
      const struct pipe_shader_buffer *b = buffers;
   SVGA_DBG(DEBUG_UAV, "%s: shader=%d start=%d num=%d ",
         if (buffers) {
      for (unsigned i = 0; i < num; i++, b++) {
            }
      #endif
         buf = buffers;
   if (buffers) {
      int last_buffer = -1;
   for (unsigned i = start, j=0; i < start + num; i++, buf++, j++) {
               if (buf && buf->buffer) {
                     /* Mark the last bound shader buffer */
      }
   else {
      cbuf->desc.buffer = NULL;
         cbuf->writeAccess = (writeable_bitmask & (1 << j)) != 0;
         }
   svga->curr.num_shader_buffers[shader] =
      }
   else {
      for (unsigned i = start; i < start + num; i++) {
      struct svga_shader_buffer *cbuf = &svga->curr.shader_buffers[shader][i];
   cbuf->desc.buffer = NULL;
   cbuf->uav_index = -1;
      }
   if ((start + num) >= svga->curr.num_shader_buffers[shader])
            #ifdef DEBUG
      SVGA_DBG(DEBUG_UAV,
            "%s: current num_shader_buffers=%d start=%d num=%d buffers=",
         for (unsigned i = start; i < start + num; i++) {
      struct svga_shader_buffer *cbuf = &svga->curr.shader_buffers[shader][i];
                  #endif
         /* purge any unused uav objects */
               }
         /**
   * Set HW atomic buffers.
   */
   static void
   svga_set_hw_atomic_buffers(struct pipe_context *pipe,
               {
      struct svga_context *svga = svga_context(pipe);
                           #ifdef DEBUG
         #endif
         buf = buffers;
   if (buffers) {
      int last_buffer = -1;
   for (unsigned i = start; i < start + num; i++, buf++) {
               if (buf && buf->buffer) {
                              /* Mark the buffer as uav buffer so that a readback will
   * be done at each read transfer. We can't rely on the
   * dirty bit because it is reset after each read, but
   * the uav buffer can be updated at each draw.
   */
   struct svga_buffer *sbuf = svga_buffer(cbuf->desc.buffer);
      }
   else {
      cbuf->desc.buffer = NULL;
      }
      }
   svga->curr.num_atomic_buffers = MAX2(svga->curr.num_atomic_buffers,
      }
   else {
      for (unsigned i = start; i < start + num; i++) {
      struct svga_shader_buffer *cbuf = &svga->curr.atomic_buffers[i];
   cbuf->desc.buffer = NULL;
   cbuf->uav_index = -1;
      }
   if ((start + num) >= svga->curr.num_atomic_buffers)
            #ifdef DEBUG
      SVGA_DBG(DEBUG_UAV, "%s: current num_atomic_buffers=%d start=%d num=%d ",
                  for (unsigned i = start; i < start + num; i++) {
      struct svga_shader_buffer *cbuf = &svga->curr.atomic_buffers[i];
                  #endif
         /* purge any unused uav objects */
               }
         /**
   *  Initialize shader images gallium interface
   */
   void
   svga_init_shader_buffer_functions(struct svga_context *svga)
   {
      if (!svga_have_gl43(svga))
            svga->pipe.set_shader_buffers = svga_set_shader_buffers;
            /* Initialize shader buffers */
   for (unsigned shader = 0; shader < PIPE_SHADER_TYPES; ++shader) {
      struct svga_shader_buffer *hw_buf =
         struct svga_shader_buffer *cur_buf =
            /* Initialize uaViewId to SVGA3D_INVALID_ID for current shader buffers
   * and shader buffers in hw state to avoid unintentional unbinding of
   * shader buffers with uaViewId 0.
   */
   for (unsigned i = 0; i < ARRAY_SIZE(svga->curr.shader_buffers[shader]);
      i++, hw_buf++, cur_buf++) {
   hw_buf->resource = NULL;
   hw_buf->uav_index = -1;
   cur_buf->desc.buffer = NULL;
   cur_buf->resource = NULL;
         }
   memset(svga->state.hw_draw.num_shader_buffers, 0,
                     /* Initialize uaViewId to SVGA3D_INVALID_ID for current atomic buffers
   * and atomic buffers in hw state to avoid unintentional unbinding of
   * shader buffer with uaViewId 0.
   */
   for (unsigned i = 0; i < ARRAY_SIZE(svga->state.hw_draw.atomic_buffers); i++) {
      svga->curr.atomic_buffers[i].resource = NULL;
      }
      }
         /**
   * Cleanup shader image state
   */
   void
   svga_cleanup_shader_buffer_state(struct svga_context *svga)
   {
      if (!svga_have_gl43(svga))
               }
         /**
   * Validate shader buffer resources to ensure any pending changes to the
   * buffers are emitted before they are referenced.
   * The helper function also rebinds the buffer resources if the rebind flag
   * is specified.
   */
   enum pipe_error
   svga_validate_shader_buffer_resources(struct svga_context *svga,
                     {
               struct svga_winsys_surface *surf;
   enum pipe_error ret;
            for (i = 0; i < count; i++) {
      if (bufs[i].resource) {
               struct svga_buffer *sbuf = svga_buffer(bufs[i].resource);
   surf = svga_buffer_handle(svga, bufs[i].desc.buffer,
         assert(surf);
   if (rebind) {
      ret = svga->swc->resource_rebind(svga->swc, surf, NULL,
         if (ret != PIPE_OK)
               /* Mark buffer as RENDERED */
                     }
         /**
   * Returns TRUE if the shader buffer can be bound to SRV as raw buffer.
   * It is TRUE if the shader buffer is readonly and the surface already
   * has the RAW_BUFFER_VIEW bind flag set.
   */
   bool
   svga_shader_buffer_can_use_srv(struct svga_context *svga,
            enum pipe_shader_type shader,
      {
      if (buf->resource) {
      struct svga_buffer *sbuf = svga_buffer(buf->resource);
   if (sbuf && !buf->writeAccess && svga_has_raw_buffer_view(sbuf)) {
            }
      }
         #define SVGA_SSBO_SRV_START  SVGA_MAX_CONST_BUFS
      /**
   * Bind the shader buffer as SRV raw buffer.
   */
   enum pipe_error
   svga_shader_buffer_bind_srv(struct svga_context *svga,
                     {
      enum pipe_error ret;
            svga->state.raw_shaderbufs[shader] |= (1 << index);
   ret = svga_emit_rawbuf(svga, slot, shader, buf->desc.buffer_offset,
         if (ret == PIPE_OK)
               }
         /**
   * Unbind the shader buffer SRV.
   */
   enum pipe_error
   svga_shader_buffer_unbind_srv(struct svga_context *svga,
                     {
      enum pipe_error ret = PIPE_OK;
            if ((svga->state.hw_draw.enabled_raw_shaderbufs[shader] & (1 << index))
            ret = svga_emit_rawbuf(svga, slot, shader, 0, 0, NULL);
   if (ret == PIPE_OK)
      }
   svga->state.raw_shaderbufs[shader] &= ~(1 << index);
      }
