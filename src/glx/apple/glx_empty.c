   #include "glxclient.h"
   #include "glxextensions.h"
   #include "glxconfig.h"
      /*
   ** GLX_SGI_swap_control
   */
   int
   glXSwapIntervalSGI(int interval)
   {
      (void) interval;
      }
         /*
   ** GLX_MESA_swap_control
   */
   int
   glXSwapIntervalMESA(unsigned int interval)
   {
      (void) interval;
      }
         int
   glXGetSwapIntervalMESA(void)
   {
         }
         /*
   ** GLX_SGI_video_sync
   */
   int
   glXGetVideoSyncSGI(unsigned int *count)
   {
      (void) count;
      }
      int
   glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
   {
      (void) count;
      }
         /*
   ** GLX_OML_sync_control
   */
   Bool
   glXGetSyncValuesOML(Display * dpy, GLXDrawable drawable,
         {
      (void) dpy;
   (void) drawable;
   (void) ust;
   (void) msc;
   (void) sbc;
      }
      int64_t
   glXSwapBuffersMscOML(Display * dpy, GLXDrawable drawable,
         {
      (void) dpy;
   (void) drawable;
   (void) target_msc;
   (void) divisor;
   (void) remainder;
      }
         Bool
   glXWaitForMscOML(Display * dpy, GLXDrawable drawable,
                     {
      (void) dpy;
   (void) drawable;
   (void) target_msc;
   (void) divisor;
   (void) remainder;
   (void) ust;
   (void) msc;
   (void) sbc;
      }
         Bool
   glXWaitForSbcOML(Display * dpy, GLXDrawable drawable,
               {
      (void) dpy;
   (void) drawable;
   (void) target_sbc;
   (void) ust;
   (void) msc;
   (void) sbc;
      }
         Bool
   glXReleaseBuffersMESA(Display * dpy, GLXDrawable d)
   {
      (void) dpy;
   (void) d;
      }
         _X_EXPORT GLXPixmap
   glXCreateGLXPixmapMESA(Display * dpy, XVisualInfo * visual,
         {
      (void) dpy;
   (void) visual;
   (void) pixmap;
   (void) cmap;
      }
         /**
   * GLX_MESA_copy_sub_buffer
   */
   void
   glXCopySubBufferMESA(Display * dpy, GLXDrawable drawable,
         {
      (void) dpy;
   (void) drawable;
   (void) x;
   (void) y;
   (void) width;
      }
         _X_EXPORT void
   glXQueryGLXPbufferSGIX(Display * dpy, GLXDrawable drawable,
         {
      (void) dpy;
   (void) drawable;
   (void) attribute;
      }
      _X_EXPORT GLXDrawable
   glXCreateGLXPbufferSGIX(Display * dpy, GLXFBConfig config,
               {
      (void) dpy;
   (void) config;
   (void) width;
   (void) height;
   (void) attrib_list;
      }
      #if 0
   /* GLX_SGIX_fbconfig */
   _X_EXPORT int
   glXGetFBConfigAttribSGIX(Display * dpy, void *config, int a, int *b)
   {
      (void) dpy;
   (void) config;
   (void) a;
   (void) b;
      }
      _X_EXPORT void *
   glXChooseFBConfigSGIX(Display * dpy, int a, int *b, int *c)
   {
      (void) dpy;
   (void) a;
   (void) b;
   (void) c;
      }
      _X_EXPORT GLXPixmap
   glXCreateGLXPixmapWithConfigSGIX(Display * dpy, void *config, Pixmap p)
   {
      (void) dpy;
   (void) config;
   (void) p;
      }
      _X_EXPORT GLXContext
   glXCreateContextWithConfigSGIX(Display * dpy, void *config, int a,
         {
      (void) dpy;
   (void) config;
   (void) a;
   (void) b;
   (void) c;
      }
      _X_EXPORT XVisualInfo *
   glXGetVisualFromFBConfigSGIX(Display * dpy, void *config)
   {
      (void) dpy;
   (void) config;
      }
      _X_EXPORT void *
   glXGetFBConfigFromVisualSGIX(Display * dpy, XVisualInfo * visinfo)
   {
      (void) dpy;
   (void) visinfo;
      }
   #endif
