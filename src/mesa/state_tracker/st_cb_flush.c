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
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Brian Paul
   */
      #include "util/glheader.h"
   #include "main/macros.h"
   #include "main/context.h"
   #include "st_context.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_flush.h"
   #include "st_cb_clear.h"
   #include "st_context.h"
   #include "st_manager.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "util/u_gen_mipmap.h"
   #include "util/perf/cpu_trace.h"
         void
   st_flush(struct st_context *st,
               {
               /* We want to call this function periodically.
   * Typically, it has nothing to do so it shouldn't be expensive.
   */
            st_flush_bitmap_cache(st);
      }
         /**
   * Flush, and wait for completion.
   */
   void
   st_finish(struct st_context *st)
   {
                                 if (fence) {
      st->screen->fence_finish(st->screen, NULL, fence,
                        }
         void
   st_glFlush(struct gl_context *ctx, unsigned gallium_flush_flags)
   {
               /* Don't call st_finish() here.  It is not the state tracker's
   * responsibilty to inject sleeps in the hope of avoiding buffer
   * synchronization issues.  Calling finish() here will just hide
   * problems that need to be fixed elsewhere.
   */
               }
      void
   st_glFinish(struct gl_context *ctx)
   {
                           }
         static GLenum
   gl_reset_status_from_pipe_reset_status(enum pipe_reset_status status)
   {
      switch (status) {
   case PIPE_NO_RESET:
         case PIPE_GUILTY_CONTEXT_RESET:
         case PIPE_INNOCENT_CONTEXT_RESET:
         case PIPE_UNKNOWN_CONTEXT_RESET:
         default:
      assert(0);
         }
         static void
   st_device_reset_callback(void *data, enum pipe_reset_status status)
   {
                        st->reset_status = status;
      }
         /**
   * Query information about GPU resets observed by this context
   *
   * Called via \c dd_function_table::GetGraphicsResetStatus.
   */
   static GLenum
   st_get_graphics_reset_status(struct gl_context *ctx)
   {
      struct st_context *st = st_context(ctx);
            if (st->reset_status != PIPE_NO_RESET) {
      status = st->reset_status;
      } else {
      status = st->pipe->get_device_reset_status(st->pipe);
   if (status != PIPE_NO_RESET)
                  }
         void
   st_install_device_reset_callback(struct st_context *st)
   {
      if (st->pipe->set_device_reset_callback) {
      struct pipe_device_reset_callback cb;
   cb.reset = st_device_reset_callback;
   cb.data = st;
         }
         void
   st_init_flush_functions(struct pipe_screen *screen,
         {
      if (screen->get_param(screen, PIPE_CAP_DEVICE_RESET_STATUS_QUERY))
      }
