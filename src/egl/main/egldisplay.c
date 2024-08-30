   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
   * Copyright 2010-2011 LunarG, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * Functions related to EGLDisplay.
   */
      #include <assert.h>
   #include <stdlib.h>
   #include <string.h>
   #ifdef _WIN32
   #include <io.h>
   #else
   #include <unistd.h>
   #endif
   #include <fcntl.h>
   #include "c11/threads.h"
   #include "util/macros.h"
   #include "util/os_file.h"
   #include "util/u_atomic.h"
      #include "eglcontext.h"
   #include "eglcurrent.h"
   #include "egldevice.h"
   #include "egldisplay.h"
   #include "egldriver.h"
   #include "eglglobals.h"
   #include "eglimage.h"
   #include "egllog.h"
   #include "eglsurface.h"
   #include "eglsync.h"
      /* Includes for _eglNativePlatformDetectNativeDisplay */
   #ifdef HAVE_WAYLAND_PLATFORM
   #include <wayland-client.h>
   #endif
   #ifdef HAVE_DRM_PLATFORM
   #include <gbm.h>
   #endif
   #ifdef HAVE_WINDOWS_PLATFORM
   #include <windows.h>
   #endif
      /**
   * Map build-system platform names to platform types.
   */
   static const struct {
      _EGLPlatformType platform;
      } egl_platforms[] = {
      {_EGL_PLATFORM_X11, "x11"},
   {_EGL_PLATFORM_XCB, "xcb"},
   {_EGL_PLATFORM_WAYLAND, "wayland"},
   {_EGL_PLATFORM_DRM, "drm"},
   {_EGL_PLATFORM_ANDROID, "android"},
   {_EGL_PLATFORM_HAIKU, "haiku"},
   {_EGL_PLATFORM_SURFACELESS, "surfaceless"},
   {_EGL_PLATFORM_DEVICE, "device"},
      };
      /**
   * Return the native platform by parsing EGL_PLATFORM.
   */
   static _EGLPlatformType
   _eglGetNativePlatformFromEnv(void)
   {
      _EGLPlatformType plat = _EGL_INVALID_PLATFORM;
   const char *plat_name;
            static_assert(ARRAY_SIZE(egl_platforms) == _EGL_NUM_PLATFORMS,
            plat_name = getenv("EGL_PLATFORM");
   /* try deprecated env variable */
   if (!plat_name || !plat_name[0])
         if (!plat_name || !plat_name[0])
            for (i = 0; i < ARRAY_SIZE(egl_platforms); i++) {
      if (strcmp(egl_platforms[i].name, plat_name) == 0) {
      plat = egl_platforms[i].platform;
                  if (plat == _EGL_INVALID_PLATFORM)
               }
      /**
   * Try detecting native platform with the help of native display characteristics.
   */
   static _EGLPlatformType
   _eglNativePlatformDetectNativeDisplay(void *nativeDisplay)
   {
      if (nativeDisplay == EGL_DEFAULT_DISPLAY)
         #ifdef HAVE_WINDOWS_PLATFORM
      if (GetObjectType(nativeDisplay) == OBJ_DC)
      #endif
      #if defined(HAVE_WAYLAND_PLATFORM) || defined(HAVE_DRM_PLATFORM)
      if (_eglPointerIsDereferenceable(nativeDisplay)) {
         #ifdef HAVE_WAYLAND_PLATFORM
         /* wl_display is a wl_proxy, which is a wl_object.
   * wl_object's first element points to the interfacetype. */
   if (first_pointer == &wl_display_interface)
   #endif
      #ifdef HAVE_DRM_PLATFORM
         /* gbm has a pointer to its constructor as first element. */
   if (first_pointer == gbm_create_device)
   #endif
         #endif
            }
      /**
   * Return the native platform.  It is the platform of the EGL native types.
   */
   _EGLPlatformType
   _eglGetNativePlatform(void *nativeDisplay)
   {
      _EGLPlatformType detected_platform = _eglGetNativePlatformFromEnv();
            if (detected_platform == _EGL_INVALID_PLATFORM) {
      detected_platform = _eglNativePlatformDetectNativeDisplay(nativeDisplay);
               if (detected_platform == _EGL_INVALID_PLATFORM) {
      detected_platform = _EGL_NATIVE_PLATFORM;
               _eglLog(_EGL_DEBUG, "Native platform type: %s (%s)",
               }
      /**
   * Finish display management.
   */
   void
   _eglFiniDisplay(void)
   {
               /* atexit function is called with global mutex locked */
   dispList = _eglGlobal.DisplayList;
   while (dispList) {
               /* pop list head */
   disp = dispList;
            for (i = 0; i < _EGL_NUM_RESOURCES; i++) {
      if (disp->ResourceLists[i]) {
      _eglLog(_EGL_DEBUG, "Display %p is destroyed with resources", disp);
                  /* The fcntl() code in _eglGetDeviceDisplay() ensures that valid fd >= 3,
   * and invalid one is 0.
   */
   if (disp->Options.fd)
            free(disp->Options.Attribs);
      }
      }
      static EGLBoolean
   _eglSameAttribs(const EGLAttrib *a, const EGLAttrib *b)
   {
      size_t na = _eglNumAttribs(a);
            /* different numbers of attributes must be different */
   if (na != nb)
            /* both lists NULL are the same */
   if (!a && !b)
            /* otherwise, compare the lists */
      }
      /**
   * Find the display corresponding to the specified native display, or create a
   * new one. EGL 1.5 says:
   *
   *     Multiple calls made to eglGetPlatformDisplay with the same parameters
   *     will return the same EGLDisplay handle.
   *
   * We read this extremely strictly, and treat a call with NULL attribs as
   * different from a call with attribs only equal to { EGL_NONE }. Similarly
   * we do not sort the attribute list, so even if all attribute _values_ are
   * identical, different attribute orders will be considered different
   * parameters.
   */
   _EGLDisplay *
   _eglFindDisplay(_EGLPlatformType plat, void *plat_dpy,
         {
      _EGLDisplay *disp;
            if (plat == _EGL_INVALID_PLATFORM)
                     /* search the display list first */
   for (disp = _eglGlobal.DisplayList; disp; disp = disp->Next) {
      if (disp->Platform == plat && disp->PlatformDisplay == plat_dpy &&
      _eglSameAttribs(disp->Options.Attribs, attrib_list))
            /* create a new display */
   assert(!disp);
   disp = calloc(1, sizeof(_EGLDisplay));
   if (!disp)
            simple_mtx_init(&disp->Mutex, mtx_plain);
   u_rwlock_init(&disp->TerminateLock);
   disp->Platform = plat;
   disp->PlatformDisplay = plat_dpy;
   num_attribs = _eglNumAttribs(attrib_list);
   if (num_attribs) {
      disp->Options.Attribs = calloc(num_attribs, sizeof(EGLAttrib));
   if (!disp->Options.Attribs) {
      free(disp);
   disp = NULL;
      }
   memcpy(disp->Options.Attribs, attrib_list,
               /* add to the display list */
   disp->Next = _eglGlobal.DisplayList;
         out:
                  }
      /**
   * Destroy the contexts and surfaces that are linked to the display.
   */
   void
   _eglReleaseDisplayResources(_EGLDisplay *display)
   {
      _EGLResource *list;
                     list = display->ResourceLists[_EGL_RESOURCE_CONTEXT];
   while (list) {
      _EGLContext *ctx = (_EGLContext *)list;
            _eglUnlinkContext(ctx);
      }
            list = display->ResourceLists[_EGL_RESOURCE_SURFACE];
   while (list) {
      _EGLSurface *surf = (_EGLSurface *)list;
            _eglUnlinkSurface(surf);
      }
            list = display->ResourceLists[_EGL_RESOURCE_IMAGE];
   while (list) {
      _EGLImage *image = (_EGLImage *)list;
            _eglUnlinkImage(image);
      }
            list = display->ResourceLists[_EGL_RESOURCE_SYNC];
   while (list) {
      _EGLSync *sync = (_EGLSync *)list;
            _eglUnlinkSync(sync);
      }
      }
      /**
   * Free all the data hanging of an _EGLDisplay object, but not
   * the object itself.
   */
   void
   _eglCleanupDisplay(_EGLDisplay *disp)
   {
      if (disp->Configs) {
      _eglDestroyArray(disp->Configs, free);
                  }
      /**
   * Return EGL_TRUE if the given resource is valid.  That is, the display does
   * own the resource.
   */
   EGLBoolean
   _eglCheckResource(void *res, _EGLResourceType type, _EGLDisplay *disp)
   {
                        if (!res)
            while (list) {
      if (res == (void *)list) {
      assert(list->Display == disp);
      }
                  }
      /**
   * Initialize a display resource.  The size of the subclass object is
   * specified.
   *
   * This is supposed to be called from the initializers of subclasses, such as
   * _eglInitContext or _eglInitSurface.
   */
   void
   _eglInitResource(_EGLResource *res, EGLint size, _EGLDisplay *disp)
   {
      memset(res, 0, size);
   res->Display = disp;
      }
      /**
   * Increment reference count for the resource.
   */
   void
   _eglGetResource(_EGLResource *res)
   {
      assert(res && res->RefCount > 0);
      }
      /**
   * Decrement reference count for the resource.
   */
   EGLBoolean
   _eglPutResource(_EGLResource *res)
   {
      assert(res && res->RefCount > 0);
      }
      /**
   * Link a resource to its display.
   */
   void
   _eglLinkResource(_EGLResource *res, _EGLResourceType type)
   {
      assert(res->Display);
            res->IsLinked = EGL_TRUE;
   res->Next = res->Display->ResourceLists[type];
   res->Display->ResourceLists[type] = res;
      }
      /**
   * Unlink a linked resource from its display.
   */
   void
   _eglUnlinkResource(_EGLResource *res, _EGLResourceType type)
   {
                        prev = res->Display->ResourceLists[type];
   if (prev != res) {
      while (prev) {
      if (prev->Next == res)
            }
   assert(prev);
      } else {
                  res->Next = NULL;
   res->IsLinked = EGL_FALSE;
            /* We always unlink before destroy.  The driver still owns a reference */
      }
      #ifdef HAVE_X11_PLATFORM
   _EGLDisplay *
   _eglGetX11Display(Display *native_display, const EGLAttrib *attrib_list)
   {
      _EGLDisplay *dpy;
            /* EGL_EXT_platform_x11 adds EGL_PLATFORM_X11_SCREEN_EXT,
   * which is optional.
   */
   if (attrib_list != NULL) {
      for (int i = 0; attrib_list[i] != EGL_NONE; i += 2) {
                     switch (attrib) {
   case EGL_DEVICE_EXT:
      dev = _eglLookupDevice((void *)value);
   if (!dev) {
      _eglError(EGL_BAD_DEVICE_EXT, "eglGetPlatformDisplay");
                  /* EGL_EXT_platform_x11 adds EGL_PLATFORM_X11_SCREEN_EXT,
   * which is optional.
   */
                  default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                     dpy = _eglFindDisplay(_EGL_PLATFORM_X11, native_display, attrib_list);
   if (dpy) {
                     }
   #endif /* HAVE_X11_PLATFORM */
      #ifdef HAVE_XCB_PLATFORM
   _EGLDisplay *
   _eglGetXcbDisplay(xcb_connection_t *native_display,
         {
      _EGLDisplay *dpy;
            /* EGL_EXT_platform_xcb recognizes exactly one attribute,
   * EGL_PLATFORM_XCB_SCREEN_EXT, which is optional.
   */
   if (attrib_list != NULL) {
      for (int i = 0; attrib_list[i] != EGL_NONE; i += 2) {
                     switch (attrib) {
   case EGL_DEVICE_EXT:
      dev = _eglLookupDevice((void *)value);
   if (!dev) {
      _eglError(EGL_BAD_DEVICE_EXT, "eglGetPlatformDisplay");
                                 default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                     dpy = _eglFindDisplay(_EGL_PLATFORM_XCB, native_display, attrib_list);
   if (dpy) {
                     }
   #endif /* HAVE_XCB_PLATFORM */
      #ifdef HAVE_DRM_PLATFORM
   _EGLDisplay *
   _eglGetGbmDisplay(struct gbm_device *native_display,
         {
      _EGLDisplay *dpy;
            /* This platform recognizes only EXT_explicit_device */
   if (attrib_list) {
      for (int i = 0; attrib_list[i] != EGL_NONE; i += 2) {
                     switch (attrib) {
   case EGL_DEVICE_EXT:
      dev = _eglLookupDevice((void *)value);
   if (!dev) {
      _eglError(EGL_BAD_DEVICE_EXT, "eglGetPlatformDisplay");
                  default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                     dpy = _eglFindDisplay(_EGL_PLATFORM_DRM, native_display, attrib_list);
   if (dpy) {
                     }
   #endif /* HAVE_DRM_PLATFORM */
      #ifdef HAVE_WAYLAND_PLATFORM
   _EGLDisplay *
   _eglGetWaylandDisplay(struct wl_display *native_display,
         {
      _EGLDisplay *dpy;
            /* This platform recognizes only EXT_explicit_device */
   if (attrib_list) {
      for (int i = 0; attrib_list[i] != EGL_NONE; i += 2) {
                     switch (attrib) {
   case EGL_DEVICE_EXT:
      dev = _eglLookupDevice((void *)value);
   if (!dev) {
      _eglError(EGL_BAD_DEVICE_EXT, "eglGetPlatformDisplay");
                  default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                     dpy = _eglFindDisplay(_EGL_PLATFORM_WAYLAND, native_display, attrib_list);
   if (dpy) {
                     }
   #endif /* HAVE_WAYLAND_PLATFORM */
      _EGLDisplay *
   _eglGetSurfacelessDisplay(void *native_display, const EGLAttrib *attrib_list)
   {
      _EGLDisplay *dpy;
            /* Any native display must be an EGLDeviceEXT we know about */
   if (native_display != NULL) {
      _eglError(EGL_BAD_PARAMETER, "eglGetPlatformDisplay");
               /* This platform recognizes only EXT_explicit_device */
   if (attrib_list) {
      for (int i = 0; attrib_list[i] != EGL_NONE; i += 2) {
                     switch (attrib) {
   case EGL_DEVICE_EXT:
      dev = _eglLookupDevice((void *)value);
   if (!dev) {
      _eglError(EGL_BAD_DEVICE_EXT, "eglGetPlatformDisplay");
                  default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                     dpy = _eglFindDisplay(_EGL_PLATFORM_SURFACELESS, NULL, attrib_list);
   if (dpy) {
                     }
      #ifdef HAVE_ANDROID_PLATFORM
   _EGLDisplay *
   _eglGetAndroidDisplay(void *native_display, const EGLAttrib *attrib_list)
   {
         /* This platform recognizes no display attributes. */
   if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                  }
   #endif /* HAVE_ANDROID_PLATFORM */
      _EGLDisplay *
   _eglGetDeviceDisplay(void *native_display, const EGLAttrib *attrib_list)
   {
      _EGLDevice *dev;
   _EGLDisplay *display;
            dev = _eglLookupDevice(native_display);
   if (!dev) {
      _eglError(EGL_BAD_PARAMETER, "eglGetPlatformDisplay");
               if (attrib_list) {
      for (int i = 0; attrib_list[i] != EGL_NONE; i += 2) {
                     /* EGL_EXT_platform_device does not recognize any attributes,
                  if (!_eglDeviceSupports(dev, _EGL_DEVICE_DRM) ||
      attrib != EGL_DRM_MASTER_FD_EXT) {
   _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
                              display = _eglFindDisplay(_EGL_PLATFORM_DEVICE, native_display, attrib_list);
   if (!display) {
      _eglError(EGL_BAD_ALLOC, "eglGetPlatformDisplay");
               /* If the fd is explicitly provided and we did not dup() it yet, do so.
   * The spec mandates that we do so, since we'll need it past the
   * eglGetPlatformDisplay call.
   *
   * The new fd is guaranteed to be 3 or greater.
   */
   if (fd != -1 && display->Options.fd == 0) {
      display->Options.fd = os_dupfd_cloexec(fd);
   if (display->Options.fd == -1) {
      /* Do not (really) need to teardown the display */
   _eglError(EGL_BAD_ALLOC, "eglGetPlatformDisplay");
                     }
