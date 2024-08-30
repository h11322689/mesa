   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file xm_api.c
   *
   * All the XMesa* API functions.
   *
   *
   * NOTES:
   *
   * The window coordinate system origin (0,0) is in the lower-left corner
   * of the window.  X11's window coordinate origin is in the upper-left
   * corner of the window.  Therefore, most drawing functions in this
   * file have to flip Y coordinates.
   *
   *
   * Byte swapping:  If the Mesa host and the X display use a different
   * byte order then there's some trickiness to be aware of when using
   * XImages.  The byte ordering used for the XImage is that of the X
   * display, not the Mesa host.
   * The color-to-pixel encoding for True/DirectColor must be done
   * according to the display's visual red_mask, green_mask, and blue_mask.
   * If XPutPixel is used to put a pixel into an XImage then XPutPixel will
   * do byte swapping if needed.  If one wants to directly "poke" the pixel
   * into the XImage's buffer then the pixel must be byte swapped first.
   *
   */
      #ifdef __CYGWIN__
   #undef WIN32
   #undef __WIN32__
   #endif
      #include <stdio.h>
   #include "xm_api.h"
   #include "xm_st.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "frontend/api.h"
      #include "util/simple_mtx.h"
   #include "util/u_atomic.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "hud/hud_context.h"
      #include "main/errors.h"
   #include "main/mtypes.h"
      #include <GL/glx.h>
      #include "state_tracker/st_context.h"
   #include "main/context.h"
      extern struct pipe_screen *
   xlib_create_screen(Display *display);
      /* Default strict invalidate to false.  This means we will not call
   * XGetGeometry after every swapbuffers, which allows swapbuffers to
   * remain asynchronous.  For apps running at 100fps with synchronous
   * swapping, a 10% boost is typical.  For gears, I see closer to 20%
   * speedup.
   *
   * Note that the work of copying data on swapbuffers doesn't disappear
   * - this change just allows the X server to execute the PutImage
   * asynchronously without us effectively blocked until its completion.
   *
   * This speeds up even llvmpipe's threaded rasterization as the
   * swapbuffers operation was a large part of the serial component of
   * an llvmpipe frame.
   *
   * The downside of this is correctness - applications which don't call
   * glViewport on window resizes will get incorrect rendering.  A
   * better solution would be to have per-frame but asynchronous
   * invalidation.  Xcb almost looks as if it could provide this, but
   * the API doesn't seem to quite be there.
   */
   DEBUG_GET_ONCE_BOOL_OPTION(xmesa_strict_invalidate, "XMESA_STRICT_INVALIDATE", false)
      bool
   xmesa_strict_invalidate(void)
   {
         }
      static int
   xmesa_get_param(struct pipe_frontend_screen *fscreen,
         {
      switch(param) {
   case ST_MANAGER_BROKEN_INVALIDATE:
         default:
            }
      /* linked list of XMesaDisplay hooks per display */
   typedef struct _XMesaExtDisplayInfo {
      struct _XMesaExtDisplayInfo *next;
   Display *display;
      } XMesaExtDisplayInfo;
      typedef struct _XMesaExtInfo {
      XMesaExtDisplayInfo *head;
      } XMesaExtInfo;
      static XMesaExtInfo MesaExtInfo;
      /* hook to delete XMesaDisplay on XDestroyDisplay */
   extern void
   xmesa_close_display(Display *display)
   {
               /* These assertions are not valid since screen creation can fail and result
   * in an empty list
   assert(MesaExtInfo.ndisplays > 0);
   assert(MesaExtInfo.head);
            _XLockMutex(_Xglobal_lock);
   /* first find display */
   prev = NULL;
   for (info = MesaExtInfo.head; info; info = info->next) {
      if (info->display == display) {
      prev = info;
                  if (info == NULL) {
      /* no display found */
   _XUnlockMutex(_Xglobal_lock);
               /* remove display entry from list */
   if (prev != MesaExtInfo.head) {
         } else {
         }
                     /* don't forget to clean up mesaDisplay */
            /**
   * XXX: Don't destroy the screens here, since there may still
   * be some dangling screen pointers that are used after this point
   * if (xmdpy->screen) {
   *    xmdpy->screen->destroy(xmdpy->screen);
   * }
            st_screen_destroy(xmdpy->fscreen);
               }
      static XMesaDisplay
   xmesa_init_display( Display *display )
   {
      static simple_mtx_t init_mutex = SIMPLE_MTX_INITIALIZER;
   XMesaDisplay xmdpy;
            if (display == NULL) {
                           /* Look for XMesaDisplay which corresponds to this display */
   info = MesaExtInfo.head;
   while(info) {
      if (info->display == display) {
      /* Found it */
   simple_mtx_unlock(&init_mutex);
      }
               /* Not found.  Create new XMesaDisplay */
            /* allocate mesa display info */
   info = (XMesaExtDisplayInfo *) Xmalloc(sizeof(XMesaExtDisplayInfo));
   if (info == NULL) {
      simple_mtx_unlock(&init_mutex);
      }
            xmdpy = &info->mesaDisplay; /* to be filled out below */
   xmdpy->display = display;
            xmdpy->fscreen = CALLOC_STRUCT(pipe_frontend_screen);
   if (!xmdpy->fscreen) {
      Xfree(info);
   simple_mtx_unlock(&init_mutex);
               xmdpy->screen = xlib_create_screen(display);
   if (!xmdpy->screen) {
      free(xmdpy->fscreen);
   Xfree(info);
   simple_mtx_unlock(&init_mutex);
               /* At this point, both fscreen and screen are known to be valid */
   xmdpy->fscreen->screen = xmdpy->screen;
   xmdpy->fscreen->get_param = xmesa_get_param;
            /* chain to the list of displays */
   _XLockMutex(_Xglobal_lock);
   info->next = MesaExtInfo.head;
   MesaExtInfo.head = info;
   MesaExtInfo.ndisplays++;
                        }
         /**********************************************************************/
   /*****                     X Utility Functions                    *****/
   /**********************************************************************/
         /**
   * Return the host's byte order as LSBFirst or MSBFirst ala X.
   */
   static int host_byte_order( void )
   {
      int i = 1;
   char *cptr = (char *) &i;
      }
               /**
   * Return the true number of bits per pixel for XImages.
   * For example, if we request a 24-bit deep visual we may actually need/get
   * 32bpp XImages.  This function returns the appropriate bpp.
   * Input:  dpy - the X display
   *         visinfo - desribes the visual to be used for XImages
   * Return:  true number of bits per pixel for XImages
   */
   static int
   bits_per_pixel( XMesaVisual xmv )
   {
      Display *dpy = xmv->display;
   XVisualInfo * visinfo = xmv->visinfo;
   XImage *img;
   int bitsPerPixel;
   /* Create a temporary XImage */
   img = XCreateImage( dpy, visinfo->visual, visinfo->depth,
         ZPixmap, 0,           /*format, offset*/
   malloc(8),    /*data*/
   1, 1,                 /*width, height*/
   32,                   /*bitmap_pad*/
   0                     /*bytes_per_line*/
   assert(img);
   /* grab the bits/pixel value */
   bitsPerPixel = img->bits_per_pixel;
   /* free the XImage */
   free( img->data );
   img->data = NULL;
   XDestroyImage( img );
      }
            /*
   * Determine if a given X window ID is valid (window exists).
   * Do this by calling XGetWindowAttributes() for the window and
   * checking if we catch an X error.
   * Input:  dpy - the display
   *         win - the window to check for existence
   * Return:  GL_TRUE - window exists
   *          GL_FALSE - window doesn't exist
   */
   static GLboolean WindowExistsFlag;
      static int window_exists_err_handler( Display* dpy, XErrorEvent* xerr )
   {
      (void) dpy;
   if (xerr->error_code == BadWindow) {
         }
      }
      static GLboolean window_exists( Display *dpy, Window win )
   {
      XWindowAttributes wa;
   int (*old_handler)( Display*, XErrorEvent* );
   WindowExistsFlag = GL_TRUE;
   old_handler = XSetErrorHandler(window_exists_err_handler);
   XGetWindowAttributes( dpy, win, &wa ); /* dummy request */
   XSetErrorHandler(old_handler);
      }
      static Status
   get_drawable_size( Display *dpy, Drawable d, uint *width, uint *height )
   {
      Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
      }
         /**
   * Return the size of the window (or pixmap) that corresponds to the
   * given XMesaBuffer.
   * \param width  returns width in pixels
   * \param height  returns height in pixels
   */
   void
   xmesa_get_window_size(Display *dpy, XMesaBuffer b,
         {
      XMesaDisplay xmdpy = xmesa_init_display(dpy);
            mtx_lock(&xmdpy->mutex);
   stat = get_drawable_size(dpy, b->ws.drawable, width, height);
            if (!stat) {
      /* probably querying a window that's recently been destroyed */
   _mesa_warning(NULL, "XGetGeometry failed!\n");
         }
      #define GET_REDMASK(__v)        __v->mesa_visual.redMask
   #define GET_GREENMASK(__v)      __v->mesa_visual.greenMask
   #define GET_BLUEMASK(__v)       __v->mesa_visual.blueMask
         /**
   * Choose the pixel format for the given visual.
   * This will tell the gallium driver how to pack pixel data into
   * drawing surfaces.
   */
   static GLuint
   choose_pixel_format(XMesaVisual v)
   {
      bool native_byte_order = (host_byte_order() ==
            if (   GET_REDMASK(v)   == 0x0000ff
      && GET_GREENMASK(v) == 0x00ff00
   && GET_BLUEMASK(v)  == 0xff0000
   && v->BitsPerPixel == 32) {
   if (native_byte_order) {
      /* no byteswapping needed */
      }
   else {
            }
   else if (   GET_REDMASK(v)   == 0xff0000
            && GET_GREENMASK(v) == 0x00ff00
   && GET_BLUEMASK(v)  == 0x0000ff
   if (native_byte_order) {
      /* no byteswapping needed */
      }
   else {
            }
   else if (   GET_REDMASK(v)   == 0x0000ff00
            && GET_GREENMASK(v) == 0x00ff0000
   && GET_BLUEMASK(v)  == 0xff000000
   if (native_byte_order) {
      /* no byteswapping needed */
      }
   else {
            }
   else if (   GET_REDMASK(v)   == 0xf800
            && GET_GREENMASK(v) == 0x07e0
   && GET_BLUEMASK(v)  == 0x001f
   && native_byte_order
   /* 5-6-5 RGB */
                  }
         /**
   * Choose a depth/stencil format that satisfies the given depth and
   * stencil sizes.
   */
   static enum pipe_format
   choose_depth_stencil_format(XMesaDisplay xmdpy, int depth, int stencil,
         {
      const enum pipe_texture_target target = PIPE_TEXTURE_2D;
   const unsigned tex_usage = PIPE_BIND_DEPTH_STENCIL;
   enum pipe_format formats[8], fmt;
                     if (depth <= 16 && stencil == 0) {
         }
   if (depth <= 24 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_X8Z24_UNORM;
      }
   if (depth <= 24 && stencil <= 8) {
      formats[count++] = PIPE_FORMAT_S8_UINT_Z24_UNORM;
      }
   if (depth <= 32 && stencil == 0) {
                  fmt = PIPE_FORMAT_NONE;
   for (i = 0; i < count; i++) {
      if (xmdpy->screen->is_format_supported(xmdpy->screen, formats[i],
                  fmt = formats[i];
                     }
            /**********************************************************************/
   /*****                Linked list of XMesaBuffers                 *****/
   /**********************************************************************/
      static XMesaBuffer XMesaBufferList = NULL;
         /**
   * Allocate a new XMesaBuffer object which corresponds to the given drawable.
   * Note that XMesaBuffer is derived from struct gl_framebuffer.
   * The new XMesaBuffer will not have any size (Width=Height=0).
   *
   * \param d  the corresponding X drawable (window or pixmap)
   * \param type  either WINDOW, PIXMAP or PBUFFER, describing d
   * \param vis  the buffer's visual
   * \param cmap  the window's colormap, if known.
   * \return new XMesaBuffer or NULL if any problem
   */
   static XMesaBuffer
   create_xmesa_buffer(Drawable d, BufferType type,
         {
      XMesaDisplay xmdpy = xmesa_init_display(vis->display);
                     if (!xmdpy)
            b = (XMesaBuffer) CALLOC_STRUCT(xmesa_buffer);
   if (!b)
            b->ws.drawable = d;
   b->ws.visual = vis->visinfo->visual;
            b->xm_visual = vis;
   b->type = type;
                     /*
   * Create framebuffer, but we'll plug in our own renderbuffers below.
   */
            /* GLX_EXT_texture_from_pixmap */
   b->TextureTarget = 0;
   b->TextureFormat = GLX_TEXTURE_FORMAT_NONE_EXT;
            /* insert buffer into linked list */
   b->Next = XMesaBufferList;
               }
         /**
   * Find an XMesaBuffer by matching X display and colormap but NOT matching
   * the notThis buffer.
   */
   XMesaBuffer
   xmesa_find_buffer(Display *dpy, Colormap cmap, XMesaBuffer notThis)
   {
      XMesaBuffer b;
   for (b = XMesaBufferList; b; b = b->Next) {
      if (b->xm_visual->display == dpy &&
      b->cmap == cmap &&
   b != notThis) {
         }
      }
         /**
   * Remove buffer from linked list, delete if no longer referenced.
   */
   static void
   xmesa_free_buffer(XMesaBuffer buffer)
   {
               for (b = XMesaBufferList; b; b = b->Next) {
      if (b == buffer) {
      /* unlink buffer from list */
   if (prev)
                        /* Since the X window for the XMesaBuffer is going away, we don't
   * want to dereference this pointer in the future.
                  /* Notify the st manager that the associated framebuffer interface
   * object is no longer valid.
                  /* XXX we should move the buffer to a delete-pending list and destroy
   * the buffer until it is no longer current.
                              }
   /* continue search */
      }
   /* buffer not found in XMesaBufferList */
      }
            /**********************************************************************/
   /*****                   Misc Private Functions                   *****/
   /**********************************************************************/
         /**
   * When a context is bound for the first time, we can finally finish
   * initializing the context's visual and buffer information.
   * \param v  the XMesaVisual to initialize
   * \param b  the XMesaBuffer to initialize (may be NULL)
   * \param window  the window/pixmap we're rendering into
   * \param cmap  the colormap associated with the window/pixmap
   * \return GL_TRUE=success, GL_FALSE=failure
   */
   static GLboolean
   initialize_visual_and_buffer(XMesaVisual v, XMesaBuffer b,
         {
               /* Save true bits/pixel */
   v->BitsPerPixel = bits_per_pixel(v);
            /* RGB WINDOW:
   * We support RGB rendering into almost any kind of visual.
   */
   const int xclass = v->visualType;
   if (xclass != GLX_TRUE_COLOR && xclass != GLX_DIRECT_COLOR) {
      _mesa_warning(NULL,
                     if (v->BitsPerPixel == 32) {
      /* We use XImages for all front/back buffers.  If an X Window or
   * X Pixmap is 32bpp, there's no guarantee that the alpha channel
   * will be preserved.  For XImages we're in luck.
   */
               /*
   * If MESA_INFO env var is set print out some debugging info
   * which can help Brian figure out what's going on when a user
   * reports bugs.
   */
   if (getenv("MESA_INFO")) {
      printf("X/Mesa visual = %p\n", (void *) v);
   printf("X/Mesa depth = %d\n", v->visinfo->depth);
                  }
            #define NUM_VISUAL_TYPES   6
      /**
   * Convert an X visual type to a GLX visual type.
   *
   * \param visualType X visual type (i.e., \c TrueColor, \c StaticGray, etc.)
   *        to be converted.
   * \return If \c visualType is a valid X visual type, a GLX visual type will
   *         be returned.  Otherwise \c GLX_NONE will be returned.
   *
   * \note
   * This code was lifted directly from lib/GL/glx/glcontextmodes.c in the
   * DRI CVS tree.
   */
   static GLint
   xmesa_convert_from_x_visual_type( int visualType )
   {
         GLX_STATIC_GRAY,  GLX_GRAY_SCALE,
   GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
   GLX_TRUE_COLOR,   GLX_DIRECT_COLOR
                  ? glx_visual_types[ visualType ] : GLX_NONE;
   }
         /**********************************************************************/
   /*****                       Public Functions                     *****/
   /**********************************************************************/
         /*
   * Create a new X/Mesa visual.
   * Input:  display - X11 display
   *         visinfo - an XVisualInfo pointer
   *         rgb_flag - GL_TRUE = RGB mode,
   *                    GL_FALSE = color index mode
   *         alpha_flag - alpha buffer requested?
   *         db_flag - GL_TRUE = double-buffered,
   *                   GL_FALSE = single buffered
   *         stereo_flag - stereo visual?
   *         ximage_flag - GL_TRUE = use an XImage for back buffer,
   *                       GL_FALSE = use an off-screen pixmap for back buffer
   *         depth_size - requested bits/depth values, or zero
   *         stencil_size - requested bits/stencil values, or zero
   *         accum_red_size - requested bits/red accum values, or zero
   *         accum_green_size - requested bits/green accum values, or zero
   *         accum_blue_size - requested bits/blue accum values, or zero
   *         accum_alpha_size - requested bits/alpha accum values, or zero
   *         num_samples - number of samples/pixel if multisampling, or zero
   *         level - visual level, usually 0
   *         visualCaveat - ala the GLX extension, usually GLX_NONE
   * Return;  a new XMesaVisual or 0 if error.
   */
   PUBLIC
   XMesaVisual XMesaCreateVisual( Display *display,
                                 XVisualInfo * visinfo,
   GLboolean rgb_flag,
   GLboolean alpha_flag,
   GLboolean db_flag,
   GLboolean stereo_flag,
   GLboolean ximage_flag,
   GLint depth_size,
   GLint stencil_size,
   GLint accum_red_size,
   GLint accum_green_size,
   {
      XMesaDisplay xmdpy = xmesa_init_display(display);
   XMesaVisual v;
            if (!xmdpy)
            if (!rgb_flag)
            /* For debugging only */
   if (getenv("MESA_XSYNC")) {
      /* This makes debugging X easier.
   * In your debugger, set a breakpoint on _XError to stop when an
   * X protocol error is generated.
   */
               v = (XMesaVisual) CALLOC_STRUCT(xmesa_visual);
   if (!v) {
                           /* Save a copy of the XVisualInfo struct because the user may Xfree()
   * the struct but we may need some of the information contained in it
   * at a later time.
   */
   v->visinfo = malloc(sizeof(*visinfo));
   if (!v->visinfo) {
      free(v);
      }
                     v->mesa_visual.redMask = visinfo->red_mask;
   v->mesa_visual.greenMask = visinfo->green_mask;
   v->mesa_visual.blueMask = visinfo->blue_mask;
   v->visualID = visinfo->visualid;
         #if !(defined(__cplusplus) || defined(c_plusplus))
         #else
         #endif
         if (alpha_flag)
                     {
      const int xclass = v->visualType;
   if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
      red_bits   = util_bitcount(GET_REDMASK(v));
   green_bits = util_bitcount(GET_GREENMASK(v));
      }
   else {
      /* this is an approximation */
   int depth;
   depth = v->visinfo->depth;
   red_bits = depth / 3;
   depth -= red_bits;
   green_bits = depth / 2;
   depth -= green_bits;
   blue_bits = depth;
   alpha_bits = 0;
      }
               /* initialize visual */
   {
               vis->doubleBufferMode = db_flag;
            vis->redBits          = red_bits;
   vis->greenBits        = green_bits;
   vis->blueBits         = blue_bits;
   vis->alphaBits        = alpha_bits;
            vis->depthBits      = depth_size;
            vis->accumRedBits   = accum_red_size;
   vis->accumGreenBits = accum_green_size;
   vis->accumBlueBits  = accum_blue_size;
                        v->stvis.buffer_mask = ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (db_flag)
         if (stereo_flag) {
      v->stvis.buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
   if (db_flag)
                        /* Check format support at requested num_samples (for multisample) */
   if (!xmdpy->screen->is_format_supported(xmdpy->screen,
                                    if (v->stvis.color_format == PIPE_FORMAT_NONE) {
      free(v->visinfo);
   free(v);
               v->stvis.depth_stencil_format =
      choose_depth_stencil_format(xmdpy, depth_size, stencil_size,
         v->stvis.accum_format = (accum_red_size +
                              }
         PUBLIC
   void XMesaDestroyVisual( XMesaVisual v )
   {
      free(v->visinfo);
      }
         /**
   * Return the informative name.
   */
   const char *
   xmesa_get_name(void)
   {
         }
         /**
   * Do per-display initializations.
   */
   int
   xmesa_init( Display *display )
   {
         }
         /**
   * Create a new XMesaContext.
   * \param v  the XMesaVisual
   * \param share_list  another XMesaContext with which to share display
   *                    lists or NULL if no sharing is wanted.
   * \return an XMesaContext or NULL if error.
   */
   PUBLIC
   XMesaContext XMesaCreateContext( XMesaVisual v, XMesaContext share_list,
               {
      XMesaDisplay xmdpy = xmesa_init_display(v->display);
   struct st_context_attribs attribs;
   enum st_context_error ctx_err = 0;
            if (!xmdpy)
            /* Note: the XMesaContext contains a Mesa struct gl_context struct (inheritance) */
   c = (XMesaContext) CALLOC_STRUCT(xmesa_context);
   if (!c)
            c->xm_visual = v;
   c->xm_buffer = NULL;   /* set later by XMesaMakeCurrent */
            memset(&attribs, 0, sizeof(attribs));
   attribs.visual = v->stvis;
   attribs.major = major;
   attribs.minor = minor;
   if (contextFlags & GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
         if (contextFlags & GLX_CONTEXT_DEBUG_BIT_ARB)
         if (contextFlags & GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB)
            switch (profileMask) {
   case GLX_CONTEXT_CORE_PROFILE_BIT_ARB:
      /* There are no profiles before OpenGL 3.2.  The
   * GLX_ARB_create_context_profile spec says:
   *
   *     "If the requested OpenGL version is less than 3.2,
   *     GLX_CONTEXT_PROFILE_MASK_ARB is ignored and the functionality
   *     of the context is determined solely by the requested version."
   */
   if (major > 3 || (major == 3 && minor >= 2)) {
      attribs.profile = API_OPENGL_CORE;
      }
      case GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB:
      /*
   * The spec also says:
   *
   *     "If version 3.1 is requested, the context returned may implement
   *     any of the following versions:
   *
   *       * Version 3.1. The GL_ARB_compatibility extension may or may not
   *         be implemented, as determined by the implementation.
   *       * The core profile of version 3.2 or greater."
   *
   * and because Mesa doesn't support GL_ARB_compatibility, the only chance to
   * honour a 3.1 context is through core profile.
   */
   if (major == 3 && minor == 1) {
         } else {
         }
      case GLX_CONTEXT_ES_PROFILE_BIT_EXT:
      if (major >= 2) {
         } else {
         }
      default:
      assert(0);
               c->st = st_api_create_context(xmdpy->fscreen, &attribs,
         if (c->st == NULL)
                     c->hud = hud_create(c->st->cso_context, NULL, c->st,
                  no_st:
         no_xmesa_context:
         }
            PUBLIC
   void XMesaDestroyContext( XMesaContext c )
   {
      if (c->hud) {
                           /* FIXME: We should destroy the screen here, but if we do so, surfaces may
   * outlive it, causing segfaults
   struct pipe_screen *screen = c->st->pipe->screen;
   screen->destroy(screen);
               }
            /**
   * Private function for creating an XMesaBuffer which corresponds to an
   * X window or pixmap.
   * \param v  the window's XMesaVisual
   * \param w  the window we're wrapping
   * \return  new XMesaBuffer or NULL if error
   */
   PUBLIC XMesaBuffer
   XMesaCreateWindowBuffer(XMesaVisual v, Window w)
   {
      XWindowAttributes attr;
   XMesaBuffer b;
   Colormap cmap;
            assert(v);
            /* Check that window depth matches visual depth */
   XGetWindowAttributes( v->display, w, &attr );
   depth = attr.depth;
   if (v->visinfo->depth != depth) {
      _mesa_warning(NULL, "XMesaCreateWindowBuffer: depth mismatch between visual (%d) and window (%d)!\n",
                     /* Find colormap */
   if (attr.colormap) {
         }
   else {
      _mesa_warning(NULL, "Window %u has no colormap!\n", (unsigned int) w);
   /* this is weird, a window w/out a colormap!? */
   /* OK, let's just allocate a new one and hope for the best */
               b = create_xmesa_buffer((Drawable) w, WINDOW, v, cmap);
   if (!b)
            if (!initialize_visual_and_buffer( v, b, (Drawable) w, cmap )) {
      xmesa_free_buffer(b);
                  }
            /**
   * Create a new XMesaBuffer from an X pixmap.
   *
   * \param v    the XMesaVisual
   * \param p    the pixmap
   * \param cmap the colormap, may be 0 if using a \c GLX_TRUE_COLOR or
   *             \c GLX_DIRECT_COLOR visual for the pixmap
   * \returns new XMesaBuffer or NULL if error
   */
   PUBLIC XMesaBuffer
   XMesaCreatePixmapBuffer(XMesaVisual v, Pixmap p, Colormap cmap)
   {
                        b = create_xmesa_buffer((Drawable) p, PIXMAP, v, cmap);
   if (!b)
            if (!initialize_visual_and_buffer(v, b, (Drawable) p, cmap)) {
      xmesa_free_buffer(b);
                  }
         /**
   * For GLX_EXT_texture_from_pixmap
   */
   XMesaBuffer
   XMesaCreatePixmapTextureBuffer(XMesaVisual v, Pixmap p,
               {
      GET_CURRENT_CONTEXT(ctx);
                     b = create_xmesa_buffer((Drawable) p, PIXMAP, v, cmap);
   if (!b)
            /* get pixmap size */
            if (target == 0) {
      /* examine dims */
   if (ctx->Extensions.ARB_texture_non_power_of_two) {
         }
   else if (   util_bitcount(b->width)  == 1
            /* power of two size */
   if (b->height == 1) {
         }
   else {
            }
   else if (ctx->Extensions.NV_texture_rectangle) {
         }
   else {
      /* non power of two textures not supported */
   XMesaDestroyBuffer(b);
                  b->TextureTarget = target;
   b->TextureFormat = format;
            if (!initialize_visual_and_buffer(v, b, (Drawable) p, cmap)) {
      xmesa_free_buffer(b);
                  }
            XMesaBuffer
   XMesaCreatePBuffer(XMesaVisual v, Colormap cmap,
         {
      Window root;
   Drawable drawable;  /* X Pixmap Drawable */
            /* allocate pixmap for front buffer */
   root = RootWindow( v->display, v->visinfo->screen );
   drawable = XCreatePixmap(v->display, root, width, height,
         if (!drawable)
            b = create_xmesa_buffer(drawable, PBUFFER, v, cmap);
   if (!b)
            if (!initialize_visual_and_buffer(v, b, drawable, cmap)) {
      xmesa_free_buffer(b);
                  }
            /*
   * Deallocate an XMesaBuffer structure and all related info.
   */
   PUBLIC void
   XMesaDestroyBuffer(XMesaBuffer b)
   {
         }
         /**
   * Notify the binding context to validate the buffer.
   */
   void
   xmesa_notify_invalid_buffer(XMesaBuffer b)
   {
         }
         /**
   * Query the current drawable size and notify the binding context.
   */
   void
   xmesa_check_buffer_size(XMesaBuffer b)
   {
               if (!b)
            if (b->type == PBUFFER)
            old_width = b->width;
                     if (b->width != old_width || b->height != old_height)
      }
         /*
   * Bind buffer b to context c and make c the current rendering context.
   */
   PUBLIC
   GLboolean XMesaMakeCurrent2( XMesaContext c, XMesaBuffer drawBuffer,
         {
               if (old_ctx && old_ctx != c) {
      XMesaFlush(old_ctx);
   old_ctx->xm_buffer = NULL;
               if (c) {
      if (!drawBuffer != !readBuffer) {
                     c->xm_buffer == drawBuffer &&
      return GL_TRUE;
            xmesa_check_buffer_size(drawBuffer);
   if (readBuffer != drawBuffer)
            c->xm_buffer = drawBuffer;
            st_api_make_current(c->st,
                  /* Solution to Stephane Rehel's problem with glXReleaseBuffersMESA(): */
   if (drawBuffer)
      }
   else {
      /* Detach */
         }
      }
         /*
   * Unbind the context c from its buffer.
   */
   GLboolean XMesaUnbindContext( XMesaContext c )
   {
      /* A no-op for XFree86 integration purposes */
      }
         XMesaContext XMesaGetCurrentContext( void )
   {
      struct st_context *st = st_api_get_current();
      }
            /**
   * Swap front and back color buffers and have winsys display front buffer.
   * If there's no front color buffer no swap actually occurs.
   */
   PUBLIC
   void XMesaSwapBuffers( XMesaBuffer b )
   {
               /* Need to draw HUD before flushing */
   if (xmctx && xmctx->hud) {
      struct pipe_resource *back =
                     if (xmctx && xmctx->xm_buffer == b) {
      struct pipe_fence_handle *fence = NULL;
   st_context_flush(xmctx->st, ST_FLUSH_FRONT, &fence, NULL, NULL);
   /* Wait until all rendering is complete */
   if (fence) {
      XMesaDisplay xmdpy = xmesa_init_display(b->xm_visual->display);
   struct pipe_screen *screen = xmdpy->screen;
   xmdpy->screen->fence_finish(screen, NULL, fence,
                                 /* TODO: remove this if the framebuffer state doesn't change. */
      }
            /*
   * Copy sub-region of back buffer to front buffer
   */
   void XMesaCopySubBuffer( XMesaBuffer b, int x, int y, int width, int height )
   {
                        xmesa_copy_st_framebuffer(b->drawable,
            }
            void XMesaFlush( XMesaContext c )
   {
      if (c && c->xm_visual->display) {
      XMesaDisplay xmdpy = xmesa_init_display(c->xm_visual->display);
            st_context_flush(c->st, ST_FLUSH_FRONT, &fence, NULL, NULL);
   if (fence) {
      xmdpy->screen->fence_finish(xmdpy->screen, NULL, fence,
            }
         }
                  XMesaBuffer XMesaFindBuffer( Display *dpy, Drawable d )
   {
      XMesaBuffer b;
   for (b = XMesaBufferList; b; b = b->Next) {
      if (b->ws.drawable == d && b->xm_visual->display == dpy) {
            }
      }
         /**
   * Free/destroy all XMesaBuffers associated with given display.
   */
   void xmesa_destroy_buffers_on_display(Display *dpy)
   {
      XMesaBuffer b, next;
   for (b = XMesaBufferList; b; b = next) {
      next = b->Next;
   if (b->xm_visual->display == dpy) {
      xmesa_free_buffer(b);
   /* delete head of list? */
   if (XMesaBufferList == b) {
                  }
         /*
   * Look for XMesaBuffers whose X window has been destroyed.
   * Deallocate any such XMesaBuffers.
   */
   void XMesaGarbageCollect( void )
   {
      XMesaBuffer b, next;
   for (b=XMesaBufferList; b; b=next) {
      next = b->Next;
   if (b->xm_visual &&
      b->xm_visual->display &&
   b->ws.drawable &&
   b->type == WINDOW) {
   XSync(b->xm_visual->display, False);
   if (!window_exists( b->xm_visual->display, b->ws.drawable )) {
      /* found a dead window, free the ancillary info */
               }
         static enum st_attachment_type xmesa_attachment_type(int glx_attachment)
   {
      switch(glx_attachment) {
      case GLX_FRONT_LEFT_EXT:
         case GLX_FRONT_RIGHT_EXT:
         case GLX_BACK_LEFT_EXT:
         case GLX_BACK_RIGHT_EXT:
         default:
      assert(0);
      }
         PUBLIC void
   XMesaBindTexImage(Display *dpy, XMesaBuffer drawable, int buffer,
         {
      struct st_context *st = st_api_get_current();
   struct pipe_frontend_drawable* pdrawable = drawable->drawable;
   struct pipe_resource *res;
   int x, y, w, h;
            x = 0;
   y = 0;
   w = drawable->width;
            /* We need to validate our attachments before using them,
   * in case the texture doesn't exist yet. */
   xmesa_st_framebuffer_validate_textures(pdrawable, w, h, 1 << st_attachment);
            if (res) {
      struct pipe_context* pipe = xmesa_get_context(pdrawable);
   enum pipe_format internal_format = res->format;
   struct pipe_transfer *tex_xfer;
   char *map;
   int line, byte_width;
                     map = pipe_texture_map(pipe, res,
                           if (!map)
            /* Grab the XImage that we want to turn into a texture. */
   img = XGetImage(dpy,
                  drawable->ws.drawable,
               if (!img) {
      pipe_texture_unmap(pipe, tex_xfer);
               /* The pipe transfer has a pitch rounded up to the nearest 64 pixels. */
            for (line = 0; line < h; line++)
      memcpy(&map[line * tex_xfer->stride],
                        st_context_teximage(st, GL_TEXTURE_2D, 0 /* level */, internal_format,
            }
            PUBLIC void
   XMesaReleaseTexImage(Display *dpy, XMesaBuffer drawable, int buffer)
   {
   }
         void
   XMesaCopyContext(XMesaContext src, XMesaContext dst, unsigned long mask)
   {
         }
