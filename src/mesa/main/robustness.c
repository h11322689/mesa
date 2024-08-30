   /*
   * Copyright © 2016 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <stdbool.h>
   #include "context.h"
   #include "debug_output.h"
   #include "get.h"
   #include "mtypes.h"
   #include "macros.h"
   #include "main/dispatch.h" /* for _gloffset_COUNT */
   #include "api_exec_decl.h"
   #include "glthread_marshal.h"
      static void GLAPIENTRY
   _context_lost_GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
   if (ctx)
            if (pname == GL_SYNC_STATUS && bufSize >= 1)
      }
      static void GLAPIENTRY
   _context_lost_GetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   if (ctx)
            if (pname == GL_QUERY_RESULT_AVAILABLE)
      }
      static int
   context_lost_nop_handler(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   if (ctx)
               }
      void
   _mesa_set_context_lost_dispatch(struct gl_context *ctx)
   {
      if (ctx->Dispatch.ContextLost == NULL) {
               ctx->Dispatch.ContextLost = malloc(numEntries * sizeof(_glapi_proc));
   if (!ctx->Dispatch.ContextLost)
            _glapi_proc *entry = (_glapi_proc *) ctx->Dispatch.ContextLost;
   unsigned i;
   for (i = 0; i < numEntries; i++)
            /* The ARB_robustness specification says:
   *
   *    "* GetError and GetGraphicsResetStatus behave normally following a
   *       graphics reset, so that the application can determine a reset
   *       has occurred, and when it is safe to destroy and recreate the
   *       context.
   *
   *     * Any commands which might cause a polling application to block
   *       indefinitely will generate a CONTEXT_LOST error, but will also
   *       return a value indicating completion to the application. Such
   *       commands include:
   *
   *        + GetSynciv with <pname> SYNC_STATUS ignores the other
   *          parameters and returns SIGNALED in <values>.
   *
   *        + GetQueryObjectuiv with <pname> QUERY_RESULT_AVAILABLE
   *          ignores the other parameters and returns TRUE in <params>."
   */
   SET_GetError(ctx->Dispatch.ContextLost, _mesa_GetError);
   SET_GetGraphicsResetStatusARB(ctx->Dispatch.ContextLost, _mesa_GetGraphicsResetStatusARB);
   SET_GetSynciv(ctx->Dispatch.ContextLost, _context_lost_GetSynciv);
               ctx->Dispatch.Current = ctx->Dispatch.ContextLost;
      }
      /**
   * Returns an error code specified by GL_ARB_robustness, or GL_NO_ERROR.
   * \return current context status
   */
   GLenum GLAPIENTRY
   _mesa_GetGraphicsResetStatusARB( void )
   {
      GET_CURRENT_CONTEXT(ctx);
            /* The ARB_robustness specification says:
   *
   *     "If the reset notification behavior is NO_RESET_NOTIFICATION_ARB,
   *     then the implementation will never deliver notification of reset
   *     events, and GetGraphicsResetStatusARB will always return NO_ERROR."
   */
   if (ctx->Const.ResetStrategy == GL_NO_RESET_NOTIFICATION_ARB) {
      if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
                                 /* Query the reset status of this context from the driver core. */
   if (ctx->Driver.GetGraphicsResetStatus)
            if (status != GL_NO_ERROR)
            if (!ctx->Driver.GetGraphicsResetStatus && (MESA_VERBOSE & VERBOSE_API))
      _mesa_debug(ctx,
                  }
