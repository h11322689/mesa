   /*
   * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
   * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
   *
   * SPDX-License-Identifier: SGI-B-2.0
   */
      /**
   * \file glxcurrent.c
   * Client-side GLX interface for current context management.
   */
      #include <pthread.h>
      #include "glxclient.h"
   #include "glapi.h"
   #include "glx_error.h"
      /*
   ** We setup some dummy structures here so that the API can be used
   ** even if no context is current.
   */
      static GLubyte dummyBuffer[__GLX_BUFFER_LIMIT_SIZE];
   static struct glx_context_vtable dummyVtable;
   /*
   ** Dummy context used by small commands when there is no current context.
   ** All the
   ** gl and glx entry points are designed to operate as nop's when using
   ** the dummy context structure.
   */
   struct glx_context dummyContext = {
      &dummyBuffer[0],
   &dummyBuffer[0],
   &dummyBuffer[0],
   &dummyBuffer[__GLX_BUFFER_LIMIT_SIZE],
   sizeof(dummyBuffer),
      };
      /*
   * Current context management and locking
   */
      _X_HIDDEN pthread_mutex_t __glXmutex = PTHREAD_MUTEX_INITIALIZER;
      /**
   * Per-thread GLX context pointer.
   *
   * \c __glXSetCurrentContext is written is such a way that this pointer can
   * \b never be \c NULL.  This is important!  Because of this
   * \c __glXGetCurrentContext can be implemented as trivial macro.
   */
   __THREAD_INITIAL_EXEC void *__glX_tls_Context = &dummyContext;
      _X_HIDDEN void
   __glXSetCurrentContext(struct glx_context * c)
   {
         }
      _X_HIDDEN void
   __glXSetCurrentContextNull(void)
   {
         #if defined(GLX_DIRECT_RENDERING)
      _glapi_set_dispatch(NULL);   /* no-op functions */
      #endif
   }
      _GLX_PUBLIC GLXContext
   glXGetCurrentContext(void)
   {
               if (cx == &dummyContext) {
         }
   else {
            }
      _GLX_PUBLIC GLXDrawable
   glXGetCurrentDrawable(void)
   {
      struct glx_context *gc = __glXGetCurrentContext();
      }
      /**
   * Make a particular context current.
   *
   * \note This is in this file so that it can access dummyContext.
   */
   static Bool
   MakeContextCurrent(Display * dpy, GLXDrawable draw,
               {
      struct glx_context *gc = (struct glx_context *) gc_user;
   struct glx_context *oldGC = __glXGetCurrentContext();
            /* Make sure that the new context has a nonzero ID.  In the request,
   * a zero context ID is used only to mean that we bind to no current
   * context.
   */
   if ((gc != NULL) && (gc->xid == None)) {
                  /* can't have only one be 0 */
   if (!!draw != !!read) {
      __glXSendError(dpy, BadMatch, None, opcode, True);
               if (oldGC == gc &&
      gc->currentDrawable == draw && gc->currentReadable == read)
                  if (oldGC != &dummyContext) {
      oldGC->vtable->unbind(oldGC);
            if (oldGC->xid == None) {
      /* We are switching away from a context that was
   * previously destroyed, so we need to free the memory
   * for the old handle. */
                           if (gc) {
      /* GLX spec 3.3: If ctx is current to some other thread, then
   * glXMakeContextCurrent will generate a BadAccess error
   */
   if (gc->currentDpy)
   {
      __glXUnlock();
   __glXSendError(dpy, BadAccess, None, opcode, True);
      }
   /* Attempt to bind the context.  We do this before mucking with
   * gc and __glXSetCurrentContext to properly handle our state in
   * case of an error.
   *
   * If an error occurs, set the Null context since we've already
   * blown away our old context.  The caller is responsible for
   * figuring out how to handle setting a valid context.
   */
   if (gc->vtable->bind(gc, draw, read) != Success) {
         } else {
      gc->currentDpy = dpy;
   gc->currentDrawable = draw;
   gc->currentReadable = read;
                           if (!ret)
               }
      _GLX_PUBLIC Bool
   glXMakeCurrent(Display * dpy, GLXDrawable draw, GLXContext gc)
   {
         }
      _GLX_PUBLIC Bool
   glXMakeContextCurrent(Display *dpy, GLXDrawable d, GLXDrawable r,
         {
         }
      _GLX_PUBLIC Bool
   glXMakeCurrentReadSGI(Display *dpy, GLXDrawable d, GLXDrawable r,
         {
         }
