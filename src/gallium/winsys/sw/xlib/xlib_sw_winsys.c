   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc., Bismarck, ND., USA
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   * 
   * 
   **************************************************************************/
      /*
   * Authors:
   *   Keith Whitwell
   *   Brian Paul
   */
      #include "util/format/u_formats.h"
   #include "pipe/p_context.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "frontend/xlibsw_api.h"
   #include "xlib_sw_winsys.h"
      #include <X11/Xlib.h>
   #include <X11/Xlibint.h>
   #include <X11/Xutil.h>
   #include <sys/ipc.h>
   #include <sys/shm.h>
   #include <X11/extensions/XShm.h>
      DEBUG_GET_ONCE_BOOL_OPTION(xlib_no_shm, "XLIB_NO_SHM", false)
      /**
   * Display target for Xlib winsys.
   * Low-level OS/window system memory buffer
   */
   struct xlib_displaytarget
   {
      enum pipe_format format;
   unsigned width;
   unsigned height;
            void *data;
            Display *display;
   Visual *visual;
   XImage *tempImage;
            /* This is the last drawable that this display target was presented
   * against.  May need to recreate gc, tempImage when this changes??
   */
            XShmSegmentInfo shminfo;
      };
         /**
   * Subclass of sw_winsys for Xlib winsys
   */
   struct xlib_sw_winsys
   {
      struct sw_winsys base;
      };
            /** Cast wrapper */
   static inline struct xlib_displaytarget *
   xlib_displaytarget(struct sw_displaytarget *dt)
   {
         }
         /**
   * X Shared Memory Image extension code
   */
      static volatile int XErrorFlag = 0;
      /**
   * Catches potential Xlib errors.
   */
   static int
   handle_xerror(Display *dpy, XErrorEvent *event)
   {
      (void) dpy;
   (void) event;
   XErrorFlag = 1;
      }
         static char *
   alloc_shm(struct xlib_displaytarget *buf, unsigned size)
   {
               shminfo->shmid = -1;
            /* 0600 = user read+write */
   shminfo->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
   if (shminfo->shmid < 0) {
                  shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
   if (shminfo->shmaddr == (char *) -1) {
      shmctl(shminfo->shmid, IPC_RMID, 0);
               shmctl(shminfo->shmid, IPC_RMID, 0);
   shminfo->readOnly = False;
      }
         /**
   * Allocate a shared memory XImage back buffer for the given display target.
   */
   static void
   alloc_shm_ximage(struct xlib_displaytarget *xlib_dt,
               {
      /*
   * We have to do a _lot_ of error checking here to be sure we can
   * really use the XSHM extension.  It seems different servers trigger
   * errors at different points if the extension won't work.  Therefore
   * we have to be very careful...
   */
            xlib_dt->tempImage = XShmCreateImage(xlib_dt->display,
                                       if (xlib_dt->tempImage == NULL) {
      shmctl(xlib_dt->shminfo.shmid, IPC_RMID, 0);
   xlib_dt->shm = False;
                  XErrorFlag = 0;
   old_handler = XSetErrorHandler(handle_xerror);
   /* This may trigger the X protocol error we're ready to catch: */
   XShmAttach(xlib_dt->display, &xlib_dt->shminfo);
            /* Mark the segment to be destroyed, so that it is automatically destroyed
   * when this process dies.  Needs to be after XShmAttach() for *BSD.
   */
            if (XErrorFlag) {
      /* we are on a remote display, this error is normal, don't print it */
   XFlush(xlib_dt->display);
   XErrorFlag = 0;
   XDestroyImage(xlib_dt->tempImage);
   xlib_dt->tempImage = NULL;
   xlib_dt->shm = False;
   (void) XSetErrorHandler(old_handler);
                  }
         static void
   alloc_ximage(struct xlib_displaytarget *xlib_dt,
               {
      /* try allocating a shared memory image first */
   if (xlib_dt->shm) {
      alloc_shm_ximage(xlib_dt, xmb, width, height);
   if (xlib_dt->tempImage)
               /* try regular (non-shared memory) image */
   xlib_dt->tempImage = XCreateImage(xlib_dt->display,
                              }
      static bool
   xlib_is_displaytarget_format_supported(struct sw_winsys *ws,
               {
      /* TODO: check visuals or other sensible thing here */
      }
         static void *
   xlib_displaytarget_map(struct sw_winsys *ws,
               {
      struct xlib_displaytarget *xlib_dt = xlib_displaytarget(dt);
   xlib_dt->mapped = xlib_dt->data;
      }
         static void
   xlib_displaytarget_unmap(struct sw_winsys *ws,
         {
      struct xlib_displaytarget *xlib_dt = xlib_displaytarget(dt);
      }
         static void
   xlib_displaytarget_destroy(struct sw_winsys *ws,
         {
               if (xlib_dt->data) {
      if (xlib_dt->shminfo.shmid >= 0) {
      shmdt(xlib_dt->shminfo.shmaddr);
   shmctl(xlib_dt->shminfo.shmid, IPC_RMID, 0);
                     xlib_dt->data = NULL;
   if (xlib_dt->tempImage)
      }
   else {
      align_free(xlib_dt->data);
   if (xlib_dt->tempImage && xlib_dt->tempImage->data == xlib_dt->data) {
         }
                  if (xlib_dt->tempImage) {
      XDestroyImage(xlib_dt->tempImage);
               if (xlib_dt->gc)
               }
         /**
   * Display/copy the image in the surface into the X window specified
   * by the display target.
   */
   static void
   xlib_sw_display(struct xlib_drawable *xlib_drawable,
               {
      static bool no_swap = false;
   static bool firsttime = true;
   struct xlib_displaytarget *xlib_dt = xlib_displaytarget(dt);
   Display *display = xlib_dt->display;
   XImage *ximage;
            if (firsttime) {
      no_swap = getenv("SP_NO_RAST") != NULL;
               if (no_swap)
            if (!box) {
      _box.width = xlib_dt->width;
   _box.height = xlib_dt->height;
               if (xlib_dt->drawable != xlib_drawable->drawable) {
      if (xlib_dt->gc) {
      XFreeGC(display, xlib_dt->gc);
               if (xlib_dt->tempImage) {
      XDestroyImage(xlib_dt->tempImage);
                           if (xlib_dt->tempImage == NULL) {
      assert(util_format_get_blockwidth(xlib_dt->format) == 1);
   assert(util_format_get_blockheight(xlib_dt->format) == 1);
   alloc_ximage(xlib_dt, xlib_drawable,
               if (!xlib_dt->tempImage)
               if (xlib_dt->gc == NULL) {
      xlib_dt->gc = XCreateGC(display, xlib_drawable->drawable, 0, NULL);
               if (xlib_dt->shm) {
      ximage = xlib_dt->tempImage;
            /* _debug_printf("XSHM\n"); */
   XShmPutImage(xlib_dt->display, xlib_drawable->drawable, xlib_dt->gc,
            }
   else {
      /* display image in Window */
   ximage = xlib_dt->tempImage;
            /* check that the XImage has been previously initialized */
   assert(ximage->format);
            /* update XImage's fields */
   ximage->width = xlib_dt->width;
   ximage->height = xlib_dt->height;
            /* _debug_printf("XPUT\n"); */
   XPutImage(xlib_dt->display, xlib_drawable->drawable, xlib_dt->gc,
                        }
         /**
   * Display/copy the image in the surface into the X window specified
   * by the display target.
   */
   static void
   xlib_displaytarget_display(struct sw_winsys *ws,
                     {
      struct xlib_drawable *xlib_drawable = (struct xlib_drawable *)context_private;
      }
         static struct sw_displaytarget *
   xlib_displaytarget_create(struct sw_winsys *winsys,
                           unsigned tex_usage,
   enum pipe_format format,
   {
      struct xlib_displaytarget *xlib_dt;
   unsigned nblocksy, size;
            xlib_dt = CALLOC_STRUCT(xlib_displaytarget);
   if (!xlib_dt)
            xlib_dt->display = ((struct xlib_sw_winsys *)winsys)->display;
   xlib_dt->format = format;
   xlib_dt->width = width;
            nblocksy = util_format_get_nblocksy(format, height);
   xlib_dt->stride = align(util_format_get_stride(format, width), alignment);
            if (!debug_get_option_xlib_no_shm() &&
      XQueryExtension(xlib_dt->display, "MIT-SHM", &ignore, &ignore, &ignore)) {
   xlib_dt->data = alloc_shm(xlib_dt, size);
   if (xlib_dt->data) {
                     if (!xlib_dt->data) {
      xlib_dt->data = align_malloc(size, alignment);
   if (!xlib_dt->data)
               *stride = xlib_dt->stride;
         no_data:
         no_xlib_dt:
         }
         static struct sw_displaytarget *
   xlib_displaytarget_from_handle(struct sw_winsys *winsys,
                     {
      assert(0);
      }
         static bool
   xlib_displaytarget_get_handle(struct sw_winsys *winsys,
               {
      assert(0);
      }
         static void
   xlib_destroy(struct sw_winsys *ws)
   {
         }
         struct sw_winsys *
   xlib_create_sw_winsys(Display *display)
   {
               ws = CALLOC_STRUCT(xlib_sw_winsys);
   if (!ws)
            ws->display = display;
                     ws->base.displaytarget_create = xlib_displaytarget_create;
   ws->base.displaytarget_from_handle = xlib_displaytarget_from_handle;
   ws->base.displaytarget_get_handle = xlib_displaytarget_get_handle;
   ws->base.displaytarget_map = xlib_displaytarget_map;
   ws->base.displaytarget_unmap = xlib_displaytarget_unmap;
                        }
