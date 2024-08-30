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
   * Public EGL API entrypoints
   *
   * Generally, we use the EGLDisplay parameter as a key to lookup the
   * appropriate device driver handle, then jump though the driver's
   * dispatch table to handle the function.
   *
   * That allows us the option of supporting multiple, simultaneous,
   * heterogeneous hardware devices in the future.
   *
   * The EGLDisplay, EGLConfig, EGLContext and EGLSurface types are
   * opaque handles. Internal objects are linked to a display to
   * create the handles.
   *
   * For each public API entry point, the opaque handles are looked up
   * before being dispatched to the drivers.  When it fails to look up
   * a handle, one of
   *
   * EGL_BAD_DISPLAY
   * EGL_BAD_CONFIG
   * EGL_BAD_CONTEXT
   * EGL_BAD_SURFACE
   * EGL_BAD_SCREEN_MESA
   * EGL_BAD_MODE_MESA
   *
   * is generated and the driver function is not called. An
   * uninitialized EGLDisplay has no driver associated with it. When
   * such display is detected,
   *
   * EGL_NOT_INITIALIZED
   *
   * is generated.
   *
   * Some of the entry points use current display, context, or surface
   * implicitly.  For such entry points, the implicit objects are also
   * checked before calling the driver function.  Other than the
   * errors listed above,
   *
   * EGL_BAD_CURRENT_SURFACE
   *
   * may also be generated.
   *
   * Notes on naming conventions:
   *
   * eglFooBar    - public EGL function
   * EGL_FOO_BAR  - public EGL token
   * EGLDatatype  - public EGL datatype
   *
   * _eglFooBar   - private EGL function
   * _EGLDatatype - private EGL datatype, typedef'd struct
   * _egl_struct  - private EGL struct, non-typedef'd
   *
   */
      #ifdef USE_LIBGLVND
   #define EGLAPI
   #undef PUBLIC
   #define PUBLIC
   #endif
      #include <assert.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include "c11/threads.h"
   #include "mapi/glapi/glapi.h"
   #include "util/macros.h"
   #include "util/perf/cpu_trace.h"
   #include "util/u_debug.h"
      #include "eglconfig.h"
   #include "eglcontext.h"
   #include "eglcurrent.h"
   #include "egldefines.h"
   #include "egldevice.h"
   #include "egldisplay.h"
   #include "egldriver.h"
   #include "eglglobals.h"
   #include "eglimage.h"
   #include "egllog.h"
   #include "eglsurface.h"
   #include "eglsync.h"
   #include "egltypedefs.h"
      #include "GL/mesa_glinterop.h"
      /**
   * Macros to help return an API entrypoint.
   *
   * These macros will unlock the display and record the error code.
   */
   #define RETURN_EGL_ERROR(disp, err, ret)                                       \
      do {                                                                        \
      if (disp)                                                                \
         /* EGL error codes are non-zero */                                       \
   if (err)                                                                 \
                  #define RETURN_EGL_SUCCESS(disp, ret) RETURN_EGL_ERROR(disp, EGL_SUCCESS, ret)
      /* record EGL_SUCCESS only when ret evaluates to true */
   #define RETURN_EGL_EVAL(disp, ret)                                             \
            /*
   * A bunch of macros and checks to simplify error checking.
   */
      #define _EGL_CHECK_DISPLAY(disp, ret)                                          \
      do {                                                                        \
      if (!_eglCheckDisplay(disp, __func__))                                   \
            #define _EGL_CHECK_OBJECT(disp, type, obj, ret)                                \
      do {                                                                        \
      if (!_eglCheck##type(disp, obj, __func__))                               \
            #define _EGL_CHECK_SURFACE(disp, surf, ret)                                    \
            #define _EGL_CHECK_CONTEXT(disp, context, ret)                                 \
            #define _EGL_CHECK_CONFIG(disp, conf, ret)                                     \
            #define _EGL_CHECK_SYNC(disp, s, ret) _EGL_CHECK_OBJECT(disp, Sync, s, ret)
      static _EGLResource **
   _egl_relax_begin(_EGLDisplay *disp, _EGLResource **rs, unsigned rs_count)
   {
      for (unsigned i = 0; i < rs_count; i++)
      if (rs[i])
      simple_mtx_unlock(&disp->Mutex);
      }
      static _EGLResource **
   _egl_relax_end(_EGLDisplay *disp, _EGLResource **rs, unsigned rs_count)
   {
      simple_mtx_lock(&disp->Mutex);
   for (unsigned i = 0; i < rs_count; i++)
      if (rs[i])
         }
      /**
   * Helper to relax (drop) the EGL BDL over it's body, optionally holding
   * a reference to a list of _EGLResource's until the lock is re-acquired,
   * protecting the resources from destruction while the BDL is dropped.
   */
   #define egl_relax(disp, ...)                                                   \
      for (_EGLResource * __rs[] = {NULL /* for vs2019 */, __VA_ARGS__},          \
                     extern const _EGLDriver _eglDriver;
      struct _egl_entrypoint {
      const char *name;
      };
      static inline bool
   _eglCheckDisplay(_EGLDisplay *disp, const char *msg)
   {
      if (!disp) {
      _eglError(EGL_BAD_DISPLAY, msg);
      }
   if (!disp->Initialized) {
      _eglError(EGL_NOT_INITIALIZED, msg);
      }
      }
      static inline bool
   _eglCheckSurface(_EGLDisplay *disp, _EGLSurface *surf, const char *msg)
   {
      if (!_eglCheckDisplay(disp, msg))
         if (!surf) {
      _eglError(EGL_BAD_SURFACE, msg);
      }
      }
      static inline bool
   _eglCheckContext(_EGLDisplay *disp, _EGLContext *context, const char *msg)
   {
      if (!_eglCheckDisplay(disp, msg))
         if (!context) {
      _eglError(EGL_BAD_CONTEXT, msg);
      }
      }
      static inline bool
   _eglCheckConfig(_EGLDisplay *disp, _EGLConfig *conf, const char *msg)
   {
      if (!_eglCheckDisplay(disp, msg))
         if (!conf) {
      _eglError(EGL_BAD_CONFIG, msg);
      }
      }
      static inline bool
   _eglCheckSync(_EGLDisplay *disp, _EGLSync *s, const char *msg)
   {
      if (!_eglCheckDisplay(disp, msg))
         if (!s) {
      _eglError(EGL_BAD_PARAMETER, msg);
      }
      }
      /**
   * Lookup a handle to find the linked display.
   * Return NULL if the handle has no corresponding linked display.
   */
   static _EGLDisplay *
   _eglLookupDisplay(EGLDisplay dpy)
   {
               _EGLDisplay *cur = _eglGlobal.DisplayList;
   while (cur) {
      if (cur == (_EGLDisplay *)dpy)
            }
               }
      /**
   * Lookup and lock a display.
   */
   _EGLDisplay *
   _eglLockDisplay(EGLDisplay dpy)
   {
      _EGLDisplay *disp = _eglLookupDisplay(dpy);
   if (disp) {
      u_rwlock_rdlock(&disp->TerminateLock);
      }
      }
      /**
   * Lookup and write-lock a display. Should only be called from
   * eglTerminate.
   */
   static _EGLDisplay *
   _eglWriteLockDisplay(EGLDisplay dpy)
   {
      _EGLDisplay *disp = _eglLookupDisplay(dpy);
   if (disp) {
      u_rwlock_wrlock(&disp->TerminateLock);
      }
      }
      /**
   * Unlock a display.
   */
   void
   _eglUnlockDisplay(_EGLDisplay *disp)
   {
      simple_mtx_unlock(&disp->Mutex);
      }
      static void
   _eglSetFuncName(const char *funcName, _EGLDisplay *disp, EGLenum objectType,
         {
      _EGLThreadInfo *thr = _eglGetCurrentThread();
   thr->CurrentFuncName = funcName;
            if (objectType == EGL_OBJECT_THREAD_KHR)
         else if (objectType == EGL_OBJECT_DISPLAY_KHR && disp)
         else if (object)
      }
      #define _EGL_FUNC_START(disp, objectType, object)                              \
      do {                                                                        \
      MESA_TRACE_FUNC();                                                       \
            /**
   * Convert an attribute list from EGLint[] to EGLAttrib[].
   *
   * Return an EGL error code. The output parameter out_attrib_list is modified
   * only on success.
   */
   static EGLint
   _eglConvertIntsToAttribs(const EGLint *int_list, EGLAttrib **out_attrib_list)
   {
      size_t len = 0;
            if (int_list) {
      while (int_list[2 * len] != EGL_NONE)
               if (len == 0) {
      *out_attrib_list = NULL;
               if (2 * len + 1 > SIZE_MAX / sizeof(EGLAttrib))
            attrib_list = malloc((2 * len + 1) * sizeof(EGLAttrib));
   if (!attrib_list)
            for (size_t i = 0; i < len; ++i) {
      attrib_list[2 * i + 0] = int_list[2 * i + 0];
                        *out_attrib_list = attrib_list;
      }
      static EGLint *
   _eglConvertAttribsToInt(const EGLAttrib *attr_list)
   {
      size_t size = _eglNumAttribs(attr_list);
            /* Convert attributes from EGLAttrib[] to EGLint[] */
   if (size) {
      int_attribs = calloc(size, sizeof(int_attribs[0]));
   if (!int_attribs)
            for (size_t i = 0; i < size; i++)
      }
      }
      /**
   * This is typically the first EGL function that an application calls.
   * It associates a private _EGLDisplay object to the native display.
   */
   PUBLIC EGLDisplay EGLAPIENTRY
   eglGetDisplay(EGLNativeDisplayType nativeDisplay)
   {
      _EGLPlatformType plat;
   _EGLDisplay *disp;
            util_cpu_trace_init();
            STATIC_ASSERT(sizeof(void *) == sizeof(nativeDisplay));
            plat = _eglGetNativePlatform(native_display_ptr);
   disp = _eglFindDisplay(plat, native_display_ptr, NULL);
      }
      static EGLDisplay
   _eglGetPlatformDisplayCommon(EGLenum platform, void *native_display,
         {
                  #ifdef HAVE_X11_PLATFORM
      case EGL_PLATFORM_X11_EXT:
      disp = _eglGetX11Display((Display *)native_display, attrib_list);
   #endif
   #ifdef HAVE_XCB_PLATFORM
      case EGL_PLATFORM_XCB_EXT:
      disp = _eglGetXcbDisplay((xcb_connection_t *)native_display, attrib_list);
   #endif
   #ifdef HAVE_DRM_PLATFORM
      case EGL_PLATFORM_GBM_MESA:
      disp =
         #endif
   #ifdef HAVE_WAYLAND_PLATFORM
      case EGL_PLATFORM_WAYLAND_EXT:
      disp = _eglGetWaylandDisplay((struct wl_display *)native_display,
         #endif
      case EGL_PLATFORM_SURFACELESS_MESA:
      disp = _eglGetSurfacelessDisplay(native_display, attrib_list);
   #ifdef HAVE_ANDROID_PLATFORM
      case EGL_PLATFORM_ANDROID_KHR:
      disp = _eglGetAndroidDisplay(native_display, attrib_list);
   #endif
      case EGL_PLATFORM_DEVICE_EXT:
      disp = _eglGetDeviceDisplay(native_display, attrib_list);
      default:
                     }
      static EGLDisplay EGLAPIENTRY
   eglGetPlatformDisplayEXT(EGLenum platform, void *native_display,
         {
      EGLAttrib *attrib_list;
            util_cpu_trace_init();
            if (_eglConvertIntsToAttribs(int_attribs, &attrib_list) != EGL_SUCCESS)
            disp = _eglGetPlatformDisplayCommon(platform, native_display, attrib_list);
   free(attrib_list);
      }
      PUBLIC EGLDisplay EGLAPIENTRY
   eglGetPlatformDisplay(EGLenum platform, void *native_display,
         {
      util_cpu_trace_init();
   _EGL_FUNC_START(NULL, EGL_OBJECT_THREAD_KHR, NULL);
      }
      /**
   * Copy the extension into the string and update the string pointer.
   */
   static EGLint
   _eglAppendExtension(char **str, const char *ext)
   {
      char *s = *str;
            if (s) {
      memcpy(s, ext, len);
   s[len++] = ' ';
               } else {
                     }
      /**
   * Examine the individual extension enable/disable flags and recompute
   * the driver's Extensions string.
   */
   static void
   _eglCreateExtensionsString(_EGLDisplay *disp)
   {
   #define _EGL_CHECK_EXTENSION(ext)                                              \
      do {                                                                        \
      if (disp->Extensions.ext) {                                              \
      _eglAppendExtension(&exts, "EGL_" #ext);                              \
                           /* Please keep these sorted alphabetically. */
   _EGL_CHECK_EXTENSION(ANDROID_blob_cache);
   _EGL_CHECK_EXTENSION(ANDROID_framebuffer_target);
   _EGL_CHECK_EXTENSION(ANDROID_image_native_buffer);
   _EGL_CHECK_EXTENSION(ANDROID_native_fence_sync);
            _EGL_CHECK_EXTENSION(CHROMIUM_sync_control);
            _EGL_CHECK_EXTENSION(EXT_buffer_age);
   _EGL_CHECK_EXTENSION(EXT_create_context_robustness);
   _EGL_CHECK_EXTENSION(EXT_image_dma_buf_import);
   _EGL_CHECK_EXTENSION(EXT_image_dma_buf_import_modifiers);
   _EGL_CHECK_EXTENSION(EXT_protected_content);
   _EGL_CHECK_EXTENSION(EXT_protected_surface);
   _EGL_CHECK_EXTENSION(EXT_present_opaque);
   _EGL_CHECK_EXTENSION(EXT_surface_CTA861_3_metadata);
   _EGL_CHECK_EXTENSION(EXT_surface_SMPTE2086_metadata);
                     _EGL_CHECK_EXTENSION(KHR_cl_event2);
   _EGL_CHECK_EXTENSION(KHR_config_attribs);
   _EGL_CHECK_EXTENSION(KHR_context_flush_control);
   _EGL_CHECK_EXTENSION(KHR_create_context);
   _EGL_CHECK_EXTENSION(KHR_create_context_no_error);
   _EGL_CHECK_EXTENSION(KHR_fence_sync);
   _EGL_CHECK_EXTENSION(KHR_get_all_proc_addresses);
   _EGL_CHECK_EXTENSION(KHR_gl_colorspace);
   _EGL_CHECK_EXTENSION(KHR_gl_renderbuffer_image);
   _EGL_CHECK_EXTENSION(KHR_gl_texture_2D_image);
   _EGL_CHECK_EXTENSION(KHR_gl_texture_3D_image);
   _EGL_CHECK_EXTENSION(KHR_gl_texture_cubemap_image);
   if (disp->Extensions.KHR_image_base && disp->Extensions.KHR_image_pixmap)
         _EGL_CHECK_EXTENSION(KHR_image);
   _EGL_CHECK_EXTENSION(KHR_image_base);
   _EGL_CHECK_EXTENSION(KHR_image_pixmap);
   _EGL_CHECK_EXTENSION(KHR_mutable_render_buffer);
   _EGL_CHECK_EXTENSION(KHR_no_config_context);
   _EGL_CHECK_EXTENSION(KHR_partial_update);
   _EGL_CHECK_EXTENSION(KHR_reusable_sync);
   _EGL_CHECK_EXTENSION(KHR_surfaceless_context);
   if (disp->Extensions.EXT_swap_buffers_with_damage)
         _EGL_CHECK_EXTENSION(EXT_pixel_format_float);
            if (disp->Extensions.KHR_no_config_context)
         _EGL_CHECK_EXTENSION(MESA_drm_image);
   _EGL_CHECK_EXTENSION(MESA_gl_interop);
   _EGL_CHECK_EXTENSION(MESA_image_dma_buf_export);
            _EGL_CHECK_EXTENSION(NOK_swap_region);
                     _EGL_CHECK_EXTENSION(WL_bind_wayland_display);
         #undef _EGL_CHECK_EXTENSION
   }
      static void
   _eglCreateAPIsString(_EGLDisplay *disp)
   {
   #define addstr(str)                                                            \
      {                                                                           \
      const size_t old_len = strlen(disp->ClientAPIsString);                   \
   const size_t add_len = sizeof(str);                                      \
   const size_t max_len = sizeof(disp->ClientAPIsString) - 1;               \
   if (old_len + add_len <= max_len)                                        \
         else                                                                     \
               if (disp->ClientAPIs & EGL_OPENGL_BIT)
            if (disp->ClientAPIs & EGL_OPENGL_ES_BIT ||
      disp->ClientAPIs & EGL_OPENGL_ES2_BIT ||
   disp->ClientAPIs & EGL_OPENGL_ES3_BIT_KHR) {
               if (disp->ClientAPIs & EGL_OPENVG_BIT)
         #undef addstr
   }
      static void
   _eglComputeVersion(_EGLDisplay *disp)
   {
               if (disp->Extensions.KHR_fence_sync && disp->Extensions.KHR_cl_event2 &&
      disp->Extensions.KHR_wait_sync && disp->Extensions.KHR_image_base &&
   disp->Extensions.KHR_gl_texture_2D_image &&
   disp->Extensions.KHR_gl_texture_3D_image &&
   disp->Extensions.KHR_gl_texture_cubemap_image &&
   disp->Extensions.KHR_gl_renderbuffer_image &&
   disp->Extensions.KHR_create_context &&
   disp->Extensions.EXT_create_context_robustness &&
   disp->Extensions.KHR_get_all_proc_addresses &&
   disp->Extensions.KHR_gl_colorspace &&
   disp->Extensions.KHR_surfaceless_context)
            #if defined(ANDROID) && ANDROID_API_LEVEL <= 28
         #endif
   }
      /**
   * This is typically the second EGL function that an application calls.
   * Here we load/initialize the actual hardware driver.
   */
   PUBLIC EGLBoolean EGLAPIENTRY
   eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
   {
                                 if (!disp)
            if (!disp->Initialized) {
      /* set options */
   disp->Options.ForceSoftware =
         if (disp->Options.ForceSoftware)
                  const char *env = getenv("MESA_LOADER_DRIVER_OVERRIDE");
            const char *gallium_hud_env = getenv("GALLIUM_HUD");
   disp->Options.GalliumHudWarn =
            /**
   * Initialize the display using the driver's function.
   * If the initialisation fails, try again using only software rendering.
   */
   if (!_eglDriver.Initialize(disp)) {
      if (disp->Options.ForceSoftware)
         else {
      bool success = false;
   if (!disp->Options.Zink && !getenv("GALLIUM_DRIVER")) {
      disp->Options.Zink = EGL_TRUE;
      }
   if (!success) {
      disp->Options.Zink = EGL_FALSE;
   disp->Options.ForceSoftware = EGL_TRUE;
   if (!_eglDriver.Initialize(disp))
                     disp->Initialized = EGL_TRUE;
            /* limit to APIs supported by core */
            /* EGL_KHR_get_all_proc_addresses is a corner-case extension. The spec
   * classifies it as an EGL display extension, though conceptually it's an
   * EGL client extension.
   *
   * From the EGL_KHR_get_all_proc_addresses spec:
   *
   *    The EGL implementation must expose the name
   *    EGL_KHR_client_get_all_proc_addresses if and only if it exposes
   *    EGL_KHR_get_all_proc_addresses and supports
   *    EGL_EXT_client_extensions.
   *
   * Mesa unconditionally exposes both client extensions mentioned above,
   * so the spec requires that each EGLDisplay unconditionally expose
   * EGL_KHR_get_all_proc_addresses also.
   */
            /* Extensions is used to provide EGL 1.3 functionality for 1.2 aware
   * programs. It is driver agnostic and handled in the main EGL code.
   */
            _eglComputeVersion(disp);
   _eglCreateExtensionsString(disp);
   _eglCreateAPIsString(disp);
   snprintf(disp->VersionString, sizeof(disp->VersionString), "%d.%d",
               /* Update applications version of major and minor if not NULL */
   if ((major != NULL) && (minor != NULL)) {
      *major = disp->Version / 10;
                  }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglTerminate(EGLDisplay dpy)
   {
                        if (!disp)
            if (disp->Initialized) {
      disp->Driver->Terminate(disp);
   /* do not reset disp->Driver */
   disp->ClientAPIsString[0] = 0;
            /* Reset blob cache funcs on terminate. */
   disp->BlobCacheSet = NULL;
               simple_mtx_unlock(&disp->Mutex);
               }
      PUBLIC const char *EGLAPIENTRY
   eglQueryString(EGLDisplay dpy, EGLint name)
   {
            #if !USE_LIBGLVND
      if (dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS) {
            #endif
         disp = _eglLockDisplay(dpy);
   _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
            switch (name) {
   case EGL_VENDOR:
         case EGL_VERSION:
         case EGL_EXTENSIONS:
         case EGL_CLIENT_APIS:
         default:
            }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                              if (!num_config)
                        }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                              if (!num_config)
                        }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
                                          }
      PUBLIC EGLContext EGLAPIENTRY
   eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLContext *share = _eglLookupContext(share_list, disp);
   _EGLContext *context;
                              if (config != EGL_NO_CONFIG_KHR)
         else if (!disp->Extensions.KHR_no_config_context)
            if (!share && share_list != EGL_NO_CONTEXT)
         else if (share && share->Resource.Display != disp) {
      /* From the spec.
   *
   * "An EGL_BAD_MATCH error is generated if an OpenGL or OpenGL ES
   *  context is requested and any of: [...]
   *
   * * share context was created on a different display
   * than the one reference by config."
   */
               context = disp->Driver->CreateContext(disp, conf, share, attrib_list);
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
                     _EGL_CHECK_CONTEXT(disp, context, EGL_FALSE);
   _eglUnlinkContext(context);
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
   _EGLSurface *draw_surf = _eglLookupSurface(draw, disp);
   _EGLSurface *read_surf = _eglLookupSurface(read, disp);
                     if (!disp)
            /* display is allowed to be uninitialized under certain condition */
   if (!disp->Initialized) {
      if (draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE ||
      ctx != EGL_NO_CONTEXT)
   }
   if (!disp->Driver)
            if (!context && ctx != EGL_NO_CONTEXT)
         if (!draw_surf || !read_surf) {
      /* From the EGL 1.4 (20130211) spec:
   *
   *    To release the current context without assigning a new one, set ctx
   *    to EGL_NO_CONTEXT and set draw and read to EGL_NO_SURFACE.
   */
   if (!disp->Extensions.KHR_surfaceless_context && ctx != EGL_NO_CONTEXT)
            if ((!draw_surf && draw != EGL_NO_SURFACE) ||
      (!read_surf && read != EGL_NO_SURFACE))
      if (draw_surf || read_surf)
               /*    If a native window underlying either draw or read is no longer valid,
   *    an EGL_BAD_NATIVE_WINDOW error is generated.
   */
   if (draw_surf && draw_surf->Lost)
         if (read_surf && read_surf->Lost)
         /* EGL_EXT_protected_surface spec says:
   *     If EGL_PROTECTED_CONTENT_EXT attributes of read is EGL_TRUE and
   *     EGL_PROTECTED_CONTENT_EXT attributes of draw is EGL_FALSE, an
   *     EGL_BAD_ACCESS error is generated.
   */
   if (read_surf && read_surf->ProtectedContent && draw_surf &&
      !draw_surf->ProtectedContent)
         egl_relax (disp, &draw_surf->Resource, &read_surf->Resource,
                           }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLContext *context = _eglLookupContext(ctx, disp);
                                          }
      /* In EGL specs 1.4 and 1.5, at the end of sections 3.5.1 and 3.5.4, it says
   * that if native_surface was already used to create a window or pixmap, we
   * can't create a new one. This is what this function checks for.
   */
   static bool
   _eglNativeSurfaceAlreadyUsed(_EGLDisplay *disp, void *native_surface)
   {
                        list = disp->ResourceLists[_EGL_RESOURCE_SURFACE];
   while (list) {
                        if (surf->Type == EGL_PBUFFER_BIT)
            if (surf->NativeSurface == native_surface)
                  }
      static EGLSurface
   _eglCreateWindowSurfaceCommon(_EGLDisplay *disp, EGLConfig config,
         {
      _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLSurface *surf = NULL;
            if (native_window == NULL)
            if (disp && (disp->Platform == _EGL_PLATFORM_SURFACELESS ||
            /* From the EGL_MESA_platform_surfaceless spec (v1):
   *
   *    eglCreatePlatformWindowSurface fails when called with a <display>
   *    that belongs to the surfaceless platform. It returns
   *    EGL_NO_SURFACE and generates EGL_BAD_NATIVE_WINDOW. The
   *    justification for this unconditional failure is that the
   *    surfaceless platform has no native windows, and therefore the
   *    <native_window> parameter is always invalid.
   *
   * This check must occur before checking the EGLConfig, which emits
   * EGL_BAD_CONFIG.
   */
                        if ((conf->SurfaceType & EGL_WINDOW_BIT) == 0)
            if (_eglNativeSurfaceAlreadyUsed(disp, native_window))
            egl_relax (disp) {
      surf = disp->Driver->CreateWindowSurface(disp, conf, native_window,
      }
               }
      PUBLIC EGLSurface EGLAPIENTRY
   eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
         {
               _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
   STATIC_ASSERT(sizeof(void *) == sizeof(window));
   return _eglCreateWindowSurfaceCommon(disp, config, (void *)window,
      }
      static void *
   _fixupNativeWindow(_EGLDisplay *disp, void *native_window)
   {
   #ifdef HAVE_X11_PLATFORM
      if (disp && disp->Platform == _EGL_PLATFORM_X11 && native_window != NULL) {
      /* The `native_window` parameter for the X11 platform differs between
   * eglCreateWindowSurface() and eglCreatePlatformPixmapSurfaceEXT(). In
   * eglCreateWindowSurface(), the type of `native_window` is an Xlib
   * `Window`. In eglCreatePlatformWindowSurfaceEXT(), the type is
   * `Window*`.  Convert `Window*` to `Window` because that's what
   * dri2_x11_create_window_surface() expects.
   */
         #endif
   #ifdef HAVE_XCB_PLATFORM
      if (disp && disp->Platform == _EGL_PLATFORM_XCB && native_window != NULL) {
      /* Similar to with X11, we need to convert (xcb_window_t *)
   * (i.e., uint32_t *) to xcb_window_t. We have to do an intermediate cast
   * to uintptr_t, since uint32_t may be smaller than a pointer.
   */
         #endif
         }
      static EGLSurface EGLAPIENTRY
   eglCreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config,
               {
                        _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
   return _eglCreateWindowSurfaceCommon(disp, config, native_window,
      }
      PUBLIC EGLSurface EGLAPIENTRY
   eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config,
               {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   EGLSurface surface;
                     int_attribs = _eglConvertAttribsToInt(attrib_list);
   if (attrib_list && !int_attribs)
            native_window = _fixupNativeWindow(disp, native_window);
   surface =
         free(int_attribs);
      }
      static void *
   _fixupNativePixmap(_EGLDisplay *disp, void *native_pixmap)
   {
   #ifdef HAVE_X11_PLATFORM
      /* The `native_pixmap` parameter for the X11 platform differs between
   * eglCreatePixmapSurface() and eglCreatePlatformPixmapSurfaceEXT(). In
   * eglCreatePixmapSurface(), the type of `native_pixmap` is an Xlib
   * `Pixmap`. In eglCreatePlatformPixmapSurfaceEXT(), the type is
   * `Pixmap*`.  Convert `Pixmap*` to `Pixmap` because that's what
   * dri2_x11_create_pixmap_surface() expects.
   */
   if (disp && disp->Platform == _EGL_PLATFORM_X11 && native_pixmap != NULL)
      #endif
   #ifdef HAVE_XCB_PLATFORM
      if (disp && disp->Platform == _EGL_PLATFORM_XCB && native_pixmap != NULL) {
      /* Similar to with X11, we need to convert (xcb_pixmap_t *)
   * (i.e., uint32_t *) to xcb_pixmap_t. We have to do an intermediate cast
   * to uintptr_t, since uint32_t may be smaller than a pointer.
   */
         #endif
         }
      static EGLSurface
   _eglCreatePixmapSurfaceCommon(_EGLDisplay *disp, EGLConfig config,
         {
      _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLSurface *surf = NULL;
            if (disp && (disp->Platform == _EGL_PLATFORM_SURFACELESS ||
            /* From the EGL_MESA_platform_surfaceless spec (v1):
   *
   *   [Like eglCreatePlatformWindowSurface,] eglCreatePlatformPixmapSurface
   *   also fails when called with a <display> that belongs to the
   *   surfaceless platform.  It returns EGL_NO_SURFACE and generates
   *   EGL_BAD_NATIVE_PIXMAP.
   *
   * This check must occur before checking the EGLConfig, which emits
   * EGL_BAD_CONFIG.
   */
                        if ((conf->SurfaceType & EGL_PIXMAP_BIT) == 0)
            if (native_pixmap == NULL)
            if (_eglNativeSurfaceAlreadyUsed(disp, native_pixmap))
            egl_relax (disp) {
      surf = disp->Driver->CreatePixmapSurface(disp, conf, native_pixmap,
      }
               }
      PUBLIC EGLSurface EGLAPIENTRY
   eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
         {
               _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
   STATIC_ASSERT(sizeof(void *) == sizeof(pixmap));
   return _eglCreatePixmapSurfaceCommon(disp, config, (void *)pixmap,
      }
      static EGLSurface EGLAPIENTRY
   eglCreatePlatformPixmapSurfaceEXT(EGLDisplay dpy, EGLConfig config,
               {
               _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
   native_pixmap = _fixupNativePixmap(disp, native_pixmap);
   return _eglCreatePixmapSurfaceCommon(disp, config, native_pixmap,
      }
      PUBLIC EGLSurface EGLAPIENTRY
   eglCreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config,
               {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   EGLSurface surface;
                     int_attribs = _eglConvertAttribsToInt(attrib_list);
   if (attrib_list && !int_attribs)
            native_pixmap = _fixupNativePixmap(disp, native_pixmap);
   surface =
         free(int_attribs);
      }
      PUBLIC EGLSurface EGLAPIENTRY
   eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLConfig *conf = _eglLookupConfig(config, disp);
   _EGLSurface *surf = NULL;
            _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
            if ((conf->SurfaceType & EGL_PBUFFER_BIT) == 0)
            egl_relax (disp) {
         }
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
   _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   _eglUnlinkSurface(surf);
   egl_relax (disp) {
                     }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
            if (disp->Driver->QuerySurface)
         else
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
                        }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
            egl_relax (disp, &surf->Resource) {
                     }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
            egl_relax (disp) {
                     }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglSwapInterval(EGLDisplay dpy, EGLint interval)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLContext *ctx = _eglGetCurrentContext();
   _EGLSurface *surf = ctx ? ctx->DrawSurface : NULL;
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
            if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT ||
      ctx->Resource.Display != disp)
         if (_eglGetSurfaceHandle(surf) == EGL_NO_SURFACE)
            if (surf->Type != EGL_WINDOW_BIT)
            interval = CLAMP(interval, surf->Config->MinSwapInterval,
            if (surf->SwapInterval != interval && disp->Driver->SwapInterval) {
      egl_relax (disp, &surf->Resource) {
            } else {
                  if (ret)
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
         /* surface must be bound to current context in EGL 1.4 */
   #ifndef _EGL_BUILT_IN_DRIVER_HAIKU
      if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT || surf != ctx->DrawSurface)
      #endif
         if (surf->Type != EGL_WINDOW_BIT)
            /* From the EGL 1.5 spec:
   *
   *    If eglSwapBuffers is called and the native window associated with
   *    surface is no longer valid, an EGL_BAD_NATIVE_WINDOW error is
   *    generated.
   */
   if (surf->Lost)
            egl_relax (disp, &surf->Resource) {
                  /* EGL_KHR_partial_update
   * Frame boundary successfully reached,
   * reset damage region and reset BufferAgeRead
   */
   if (ret) {
      surf->SetDamageRegionCalled = EGL_FALSE;
                  }
      static EGLBoolean
   _eglSwapBuffersWithDamageCommon(_EGLDisplay *disp, _EGLSurface *surf,
         {
      _EGLContext *ctx = _eglGetCurrentContext();
                     /* surface must be bound to current context in EGL 1.4 */
   if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT || surf != ctx->DrawSurface)
            if (surf->Type != EGL_WINDOW_BIT)
            if ((n_rects > 0 && rects == NULL) || n_rects < 0)
            egl_relax (disp, &surf->Resource) {
                  /* EGL_KHR_partial_update
   * Frame boundary successfully reached,
   * reset damage region and reset BufferAgeRead
   */
   if (ret) {
      surf->SetDamageRegionCalled = EGL_FALSE;
                  }
      static EGLBoolean EGLAPIENTRY
   eglSwapBuffersWithDamageEXT(EGLDisplay dpy, EGLSurface surface,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
      }
      static EGLBoolean EGLAPIENTRY
   eglSwapBuffersWithDamageKHR(EGLDisplay dpy, EGLSurface surface,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
      }
      /**
   * Clamp the rectangles so that they lie within the surface.
   */
      static void
   _eglSetDamageRegionKHRClampRects(_EGLSurface *surf, EGLint *rects,
         {
      EGLint i;
   EGLint surf_height = surf->Height;
            for (i = 0; i < (4 * n_rects); i += 4) {
      EGLint x1, y1, x2, y2;
   x1 = rects[i];
   y1 = rects[i + 1];
   x2 = rects[i + 2] + x1;
            rects[i] = CLAMP(x1, 0, surf_width);
   rects[i + 1] = CLAMP(y1, 0, surf_height);
   rects[i + 2] = CLAMP(x2, 0, surf_width) - rects[i];
         }
      static EGLBoolean EGLAPIENTRY
   eglSetDamageRegionKHR(EGLDisplay dpy, EGLSurface surface, EGLint *rects,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
   _EGLContext *ctx = _eglGetCurrentContext();
   EGLBoolean ret;
            if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT ||
      surf->Type != EGL_WINDOW_BIT || ctx->DrawSurface != surf ||
   surf->SwapBehavior != EGL_BUFFER_DESTROYED)
         /* If the damage region is already set or
   * buffer age is not queried between
   * frame boundaries, throw bad access error
            if (surf->SetDamageRegionCalled || !surf->BufferAgeRead)
            _eglSetDamageRegionKHRClampRects(surf, rects, n_rects);
            if (ret)
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
   EGLBoolean ret = EGL_FALSE;
            _EGL_FUNC_START(disp, EGL_OBJECT_SURFACE_KHR, surf);
   STATIC_ASSERT(sizeof(void *) == sizeof(target));
            _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   if (surf->ProtectedContent)
            egl_relax (disp, &surf->Resource) {
                     }
      static EGLBoolean
   _eglWaitClientCommon(void)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp;
            if (!ctx)
                     /* let bad current context imply bad current surface */
   if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT ||
      _eglGetSurfaceHandle(ctx->DrawSurface) == EGL_NO_SURFACE)
         /* a valid current context implies an initialized current display */
            egl_relax (disp, &ctx->Resource) {
                     }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglWaitClient(void)
   {
      _EGL_FUNC_START(NULL, EGL_OBJECT_CONTEXT_KHR, _eglGetCurrentContext());
      }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglWaitGL(void)
   {
      /* Since we only support OpenGL and GLES, eglWaitGL is equivalent to
   * eglWaitClient. */
   _EGL_FUNC_START(NULL, EGL_OBJECT_CONTEXT_KHR, _eglGetCurrentContext());
      }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglWaitNative(EGLint engine)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp;
            if (!ctx)
                              /* let bad current context imply bad current surface */
   if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT ||
      _eglGetSurfaceHandle(ctx->DrawSurface) == EGL_NO_SURFACE)
         /* a valid current context implies an initialized current display */
            egl_relax (disp) {
                     }
      PUBLIC EGLDisplay EGLAPIENTRY
   eglGetCurrentDisplay(void)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
                        }
      PUBLIC EGLContext EGLAPIENTRY
   eglGetCurrentContext(void)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
                        }
      PUBLIC EGLSurface EGLAPIENTRY
   eglGetCurrentSurface(EGLint readdraw)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   EGLint err = EGL_SUCCESS;
   _EGLSurface *surf;
                     if (!ctx)
            switch (readdraw) {
   case EGL_DRAW:
      surf = ctx->DrawSurface;
      case EGL_READ:
      surf = ctx->ReadSurface;
      default:
      surf = NULL;
   err = EGL_BAD_PARAMETER;
                           }
      PUBLIC EGLint EGLAPIENTRY
   eglGetError(void)
   {
      _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLint e = t->LastError;
   t->LastError = EGL_SUCCESS;
      }
      /**
   ** EGL 1.2
   **/
      /**
   * Specify the client API to use for subsequent calls including:
   *  eglCreateContext()
   *  eglGetCurrentContext()
   *  eglGetCurrentDisplay()
   *  eglGetCurrentSurface()
   *  eglMakeCurrent(when the ctx parameter is EGL NO CONTEXT)
   *  eglWaitClient()
   *  eglWaitNative()
   * See section 3.7 "Rendering Context" in the EGL specification for details.
   */
   PUBLIC EGLBoolean EGLAPIENTRY
   eglBindAPI(EGLenum api)
   {
                                 if (!_eglIsApiValid(api))
                        }
      /**
   * Return the last value set with eglBindAPI().
   */
   PUBLIC EGLenum EGLAPIENTRY
   eglQueryAPI(void)
   {
      _EGLThreadInfo *t = _eglGetCurrentThread();
            /* returns one of EGL_OPENGL_API, EGL_OPENGL_ES_API or EGL_OPENVG_API */
               }
      PUBLIC EGLSurface EGLAPIENTRY
   eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype,
               {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                              /* OpenVG is not supported */
      }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglReleaseThread(void)
   {
      /* unbind current contexts */
   _EGLThreadInfo *t = _eglGetCurrentThread();
                     if (ctx) {
               u_rwlock_rdlock(&disp->TerminateLock);
   (void)disp->Driver->MakeCurrent(disp, NULL, NULL, NULL);
                           }
      static EGLImage
   _eglCreateImageCommon(_EGLDisplay *disp, EGLContext ctx, EGLenum target,
         {
      _EGLContext *context = _eglLookupContext(ctx, disp);
   _EGLImage *img = NULL;
            _EGL_CHECK_DISPLAY(disp, EGL_NO_IMAGE_KHR);
   if (!disp->Extensions.KHR_image_base)
         if (!context && ctx != EGL_NO_CONTEXT)
         /* "If <target> is EGL_LINUX_DMA_BUF_EXT, <dpy> must be a valid display,
   *  <ctx> must be EGL_NO_CONTEXT..."
   */
   if (ctx != EGL_NO_CONTEXT && target == EGL_LINUX_DMA_BUF_EXT)
            egl_relax (disp, &context->Resource) {
      img =
                           }
      static EGLImage EGLAPIENTRY
   eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
      }
      PUBLIC EGLImage EGLAPIENTRY
   eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   EGLImage image;
                     int_attribs = _eglConvertAttribsToInt(attr_list);
   if (attr_list && !int_attribs)
            image = _eglCreateImageCommon(disp, ctx, target, buffer, int_attribs);
   free(int_attribs);
      }
      static EGLBoolean
   _eglDestroyImageCommon(_EGLDisplay *disp, _EGLImage *img)
   {
               _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
   if (!disp->Extensions.KHR_image_base)
         if (!img)
            _eglUnlinkImage(img);
               }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglDestroyImage(EGLDisplay dpy, EGLImage image)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img = _eglLookupImage(image, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_IMAGE_KHR, img);
      }
      static EGLBoolean EGLAPIENTRY
   eglDestroyImageKHR(EGLDisplay dpy, EGLImage image)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img = _eglLookupImage(image, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_IMAGE_KHR, img);
      }
      static EGLSync
   _eglCreateSync(_EGLDisplay *disp, EGLenum type, const EGLAttrib *attrib_list,
         {
      _EGLContext *ctx = _eglGetCurrentContext();
   _EGLSync *sync = NULL;
                     if (!disp->Extensions.KHR_cl_event2 && orig_is_EGLAttrib) {
      /* There exist two EGLAttrib variants of eglCreateSync*:
   * eglCreateSync64KHR which requires EGL_KHR_cl_event2, and eglCreateSync
   * which requires EGL 1.5. Here we use the presence of EGL_KHR_cl_event2
   * support as a proxy for EGL 1.5 support, even though that's not
   * entirely correct (though _eglComputeVersion does the same).
   *
   * The EGL spec provides no guidance on how to handle unsupported
   * functions. EGL_BAD_MATCH seems reasonable.
   */
               /* If type is EGL_SYNC_FENCE and no context is current for the bound API
   * (i.e., eglGetCurrentContext returns EGL_NO_CONTEXT ), an EGL_BAD_MATCH
   * error is generated.
   */
   if (!ctx &&
      (type == EGL_SYNC_FENCE_KHR || type == EGL_SYNC_NATIVE_FENCE_ANDROID))
         if (ctx && (ctx->Resource.Display != disp))
            switch (type) {
   case EGL_SYNC_FENCE_KHR:
      if (!disp->Extensions.KHR_fence_sync)
            case EGL_SYNC_REUSABLE_KHR:
      if (!disp->Extensions.KHR_reusable_sync)
            case EGL_SYNC_CL_EVENT_KHR:
      if (!disp->Extensions.KHR_cl_event2)
            case EGL_SYNC_NATIVE_FENCE_ANDROID:
      if (!disp->Extensions.ANDROID_native_fence_sync)
            default:
                  egl_relax (disp) {
                              }
      static EGLSync EGLAPIENTRY
   eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *int_list)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
            EGLSync sync;
   EGLAttrib *attrib_list;
            if (sizeof(int_list[0]) == sizeof(attrib_list[0])) {
         } else {
      err = _eglConvertIntsToAttribs(int_list, &attrib_list);
   if (err != EGL_SUCCESS)
                        if (sizeof(int_list[0]) != sizeof(attrib_list[0]))
            /* Don't double-unlock the display. _eglCreateSync already unlocked it. */
      }
      static EGLSync EGLAPIENTRY
   eglCreateSync64KHR(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
      }
      PUBLIC EGLSync EGLAPIENTRY
   eglCreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGL_FUNC_START(disp, EGL_OBJECT_DISPLAY_KHR, NULL);
      }
      static EGLBoolean
   _eglDestroySync(_EGLDisplay *disp, _EGLSync *s)
   {
               _EGL_CHECK_SYNC(disp, s, EGL_FALSE);
   assert(disp->Extensions.KHR_reusable_sync ||
                           egl_relax (disp) {
                     }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglDestroySync(EGLDisplay dpy, EGLSync sync)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SYNC_KHR, s);
      }
      static EGLBoolean EGLAPIENTRY
   eglDestroySyncKHR(EGLDisplay dpy, EGLSync sync)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SYNC_KHR, s);
      }
      static EGLint
   _eglClientWaitSyncCommon(_EGLDisplay *disp, _EGLSync *s, EGLint flags,
         {
               _EGL_CHECK_SYNC(disp, s, EGL_FALSE);
   assert(disp->Extensions.KHR_reusable_sync ||
                  if (s->SyncStatus == EGL_SIGNALED_KHR)
            egl_relax (disp, &s->Resource) {
                     }
      PUBLIC EGLint EGLAPIENTRY
   eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SYNC_KHR, s);
      }
      static EGLint EGLAPIENTRY
   eglClientWaitSyncKHR(EGLDisplay dpy, EGLSync sync, EGLint flags,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SYNC_KHR, s);
      }
      static EGLint
   _eglWaitSyncCommon(_EGLDisplay *disp, _EGLSync *s, EGLint flags)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
            _EGL_CHECK_SYNC(disp, s, EGL_FALSE);
            if (ctx == EGL_NO_CONTEXT)
            /* the API doesn't allow any flags yet */
   if (flags != 0)
            egl_relax (disp, &s->Resource) {
                     }
      static EGLint EGLAPIENTRY
   eglWaitSyncKHR(EGLDisplay dpy, EGLSync sync, EGLint flags)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SYNC_KHR, s);
      }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags)
   {
      /* The KHR version returns EGLint, while the core version returns
   * EGLBoolean. In both cases, the return values can only be EGL_FALSE and
   * EGL_TRUE.
   */
   _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   _EGL_FUNC_START(disp, EGL_OBJECT_SYNC_KHR, s);
      }
      static EGLBoolean EGLAPIENTRY
   eglSignalSyncKHR(EGLDisplay dpy, EGLSync sync, EGLenum mode)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
                     _EGL_CHECK_SYNC(disp, s, EGL_FALSE);
            egl_relax (disp, &s->Resource) {
                     }
      static EGLBoolean
   _eglGetSyncAttribCommon(_EGLDisplay *disp, _EGLSync *s, EGLint attribute,
         {
               _EGL_CHECK_SYNC(disp, s, EGL_FALSE);
   assert(disp->Extensions.KHR_reusable_sync ||
                              }
      PUBLIC EGLBoolean EGLAPIENTRY
   eglGetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
            if (!value)
               }
      static EGLBoolean EGLAPIENTRY
   eglGetSyncAttribKHR(EGLDisplay dpy, EGLSync sync, EGLint attribute,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
   EGLAttrib attrib;
                     if (!value)
            attrib = *value;
            /* The EGL_KHR_fence_sync spec says this about eglGetSyncAttribKHR:
   *
   *    If any error occurs, <*value> is not modified.
   */
   if (result == EGL_FALSE)
            *value = attrib;
      }
      static EGLint EGLAPIENTRY
   eglDupNativeFenceFDANDROID(EGLDisplay dpy, EGLSync sync)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSync *s = _eglLookupSync(sync, disp);
                     /* the spec doesn't seem to specify what happens if the fence
   * type is not EGL_SYNC_NATIVE_FENCE_ANDROID, but this seems
   * sensible:
   */
   if (!(s && (s->Type == EGL_SYNC_NATIVE_FENCE_ANDROID)))
            _EGL_CHECK_SYNC(disp, s, EGL_NO_NATIVE_FENCE_FD_ANDROID);
            egl_relax (disp, &s->Resource) {
                     }
      static EGLBoolean EGLAPIENTRY
   eglSwapBuffersRegionNOK(EGLDisplay dpy, EGLSurface surface, EGLint numRects,
         {
      _EGLContext *ctx = _eglGetCurrentContext();
   _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
                              if (!disp->Extensions.NOK_swap_region)
            /* surface must be bound to current context in EGL 1.4 */
   if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT || surf != ctx->DrawSurface)
            egl_relax (disp, &surf->Resource) {
                     }
      static EGLImage EGLAPIENTRY
   eglCreateDRMImageMESA(EGLDisplay dpy, const EGLint *attr_list)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img;
                     _EGL_CHECK_DISPLAY(disp, EGL_NO_IMAGE_KHR);
   if (!disp->Extensions.MESA_drm_image)
            img = disp->Driver->CreateDRMImageMESA(disp, attr_list);
               }
      static EGLBoolean EGLAPIENTRY
   eglExportDRMImageMESA(EGLDisplay dpy, EGLImage image, EGLint *name,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img = _eglLookupImage(image, disp);
                     _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
            if (!img)
            egl_relax (disp, &img->Resource) {
                     }
      struct wl_display;
      static EGLBoolean EGLAPIENTRY
   eglBindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                     _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
            if (!display)
            egl_relax (disp) {
                     }
      static EGLBoolean EGLAPIENTRY
   eglUnbindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                     _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
            if (!display)
            egl_relax (disp) {
                     }
      static EGLBoolean EGLAPIENTRY
   eglQueryWaylandBufferWL(EGLDisplay dpy, struct wl_resource *buffer,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                     _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
            if (!buffer)
            egl_relax (disp) {
                     }
      static struct wl_buffer *EGLAPIENTRY
   eglCreateWaylandBufferFromImageWL(EGLDisplay dpy, EGLImage image)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img;
                     _EGL_CHECK_DISPLAY(disp, NULL);
   if (!disp->Extensions.WL_create_wayland_buffer_from_image)
                     if (!img)
                        }
      static EGLBoolean EGLAPIENTRY
   eglPostSubBufferNV(EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
                              if (!disp->Extensions.NV_post_sub_buffer)
            egl_relax (disp, &surf->Resource) {
                     }
      static EGLBoolean EGLAPIENTRY
   eglGetSyncValuesCHROMIUM(EGLDisplay dpy, EGLSurface surface, EGLuint64KHR *ust,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
                     _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   if (!disp->Extensions.CHROMIUM_sync_control)
            if (!ust || !msc || !sbc)
            egl_relax (disp, &surf->Resource) {
                     }
      static EGLBoolean EGLAPIENTRY
   eglGetMscRateANGLE(EGLDisplay dpy, EGLSurface surface, EGLint *numerator,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLSurface *surf = _eglLookupSurface(surface, disp);
                     _EGL_CHECK_SURFACE(disp, surf, EGL_FALSE);
   if (!disp->Extensions.ANGLE_sync_control_rate)
            if (!numerator || !denominator)
                        }
      static EGLBoolean EGLAPIENTRY
   eglExportDMABUFImageQueryMESA(EGLDisplay dpy, EGLImage image, EGLint *fourcc,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img = _eglLookupImage(image, disp);
                     _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
            if (!img)
            egl_relax (disp, &img->Resource) {
      ret = disp->Driver->ExportDMABUFImageQueryMESA(disp, img, fourcc, nplanes,
                  }
      static EGLBoolean EGLAPIENTRY
   eglExportDMABUFImageMESA(EGLDisplay dpy, EGLImage image, int *fds,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
   _EGLImage *img = _eglLookupImage(image, disp);
                     _EGL_CHECK_DISPLAY(disp, EGL_FALSE);
            if (!img)
            egl_relax (disp, &img->Resource) {
      ret =
                  }
      static EGLint EGLAPIENTRY
   eglLabelObjectKHR(EGLDisplay dpy, EGLenum objectType, EGLObjectKHR object,
         {
      _EGLDisplay *disp = NULL;
                     if (objectType == EGL_OBJECT_THREAD_KHR) {
               t->Label = label;
               disp = _eglLockDisplay(dpy);
   if (disp == NULL)
            if (objectType == EGL_OBJECT_DISPLAY_KHR) {
      if (dpy != (EGLDisplay)object)
            disp->Label = label;
               switch (objectType) {
   case EGL_OBJECT_CONTEXT_KHR:
      type = _EGL_RESOURCE_CONTEXT;
      case EGL_OBJECT_SURFACE_KHR:
      type = _EGL_RESOURCE_SURFACE;
      case EGL_OBJECT_IMAGE_KHR:
      type = _EGL_RESOURCE_IMAGE;
      case EGL_OBJECT_SYNC_KHR:
      type = _EGL_RESOURCE_SYNC;
      case EGL_OBJECT_STREAM_KHR:
   default:
                  if (_eglCheckResource(object, type, disp)) {
               res->Label = label;
                  }
      static EGLint EGLAPIENTRY
   eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback,
         {
                                 newEnabled = _eglGlobal.debugTypesEnabled;
   if (attrib_list != NULL) {
               for (i = 0; attrib_list[i] != EGL_NONE; i += 2) {
      switch (attrib_list[i]) {
   case EGL_DEBUG_MSG_CRITICAL_KHR:
   case EGL_DEBUG_MSG_ERROR_KHR:
   case EGL_DEBUG_MSG_WARN_KHR:
   case EGL_DEBUG_MSG_INFO_KHR:
      if (attrib_list[i + 1])
         else
            default:
      // On error, set the last error code, call the current
   // debug callback, and return the error code.
   simple_mtx_unlock(_eglGlobal.Mutex);
   _eglDebugReport(EGL_BAD_ATTRIBUTE, NULL, EGL_DEBUG_MSG_ERROR_KHR,
                                 if (callback != NULL) {
      _eglGlobal.debugCallback = callback;
      } else {
      _eglGlobal.debugCallback = NULL;
   _eglGlobal.debugTypesEnabled =
               simple_mtx_unlock(_eglGlobal.Mutex);
      }
      static EGLBoolean EGLAPIENTRY
   eglQueryDebugKHR(EGLint attribute, EGLAttrib *value)
   {
                        switch (attribute) {
   case EGL_DEBUG_MSG_CRITICAL_KHR:
   case EGL_DEBUG_MSG_ERROR_KHR:
   case EGL_DEBUG_MSG_WARN_KHR:
   case EGL_DEBUG_MSG_INFO_KHR:
      if (_eglGlobal.debugTypesEnabled & DebugBitFromType(attribute))
         else
            case EGL_DEBUG_CALLBACK_KHR:
      *value = (EGLAttrib)_eglGlobal.debugCallback;
      default:
      simple_mtx_unlock(_eglGlobal.Mutex);
   _eglDebugReport(EGL_BAD_ATTRIBUTE, NULL, EGL_DEBUG_MSG_ERROR_KHR,
                     simple_mtx_unlock(_eglGlobal.Mutex);
      }
      static int
   _eglFunctionCompare(const void *key, const void *elem)
   {
      const char *procname = key;
   const struct _egl_entrypoint *entrypoint = elem;
      }
      static EGLBoolean EGLAPIENTRY
   eglQueryDmaBufFormatsEXT(EGLDisplay dpy, EGLint max_formats, EGLint *formats,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                              egl_relax (disp) {
      ret = disp->Driver->QueryDmaBufFormatsEXT(disp, max_formats, formats,
                  }
      static EGLBoolean EGLAPIENTRY
   eglQueryDmaBufModifiersEXT(EGLDisplay dpy, EGLint format, EGLint max_modifiers,
               {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                              egl_relax (disp) {
      ret = disp->Driver->QueryDmaBufModifiersEXT(
                  }
      static void EGLAPIENTRY
   eglSetBlobCacheFuncsANDROID(EGLDisplay *dpy, EGLSetBlobFuncANDROID set,
         {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
                     if (!set || !get)
            if (disp->BlobCacheSet)
            disp->BlobCacheSet = set;
                        }
      static EGLBoolean EGLAPIENTRY
   eglQueryDeviceAttribEXT(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value)
   {
      _EGLDevice *dev = _eglLookupDevice(device);
            _EGL_FUNC_START(NULL, EGL_NONE, NULL);
   if (!dev)
            ret = _eglQueryDeviceAttribEXT(dev, attribute, value);
      }
      static const char *EGLAPIENTRY
   eglQueryDeviceStringEXT(EGLDeviceEXT device, EGLint name)
   {
               _EGL_FUNC_START(NULL, EGL_NONE, NULL);
   if (!dev)
               }
      static EGLBoolean EGLAPIENTRY
   eglQueryDevicesEXT(EGLint max_devices, EGLDeviceEXT *devices,
         {
               _EGL_FUNC_START(NULL, EGL_NONE, NULL);
   ret = _eglQueryDevicesEXT(max_devices, (_EGLDevice **)devices, num_devices);
      }
      static EGLBoolean EGLAPIENTRY
   eglQueryDisplayAttribEXT(EGLDisplay dpy, EGLint attribute, EGLAttrib *value)
   {
               _EGL_FUNC_START(NULL, EGL_NONE, NULL);
            switch (attribute) {
   case EGL_DEVICE_EXT:
      *value = (EGLAttrib)disp->Device;
      default:
         }
      }
      static char *EGLAPIENTRY
   eglGetDisplayDriverConfig(EGLDisplay dpy)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
            _EGL_FUNC_START(disp, EGL_NONE, NULL);
                     ret = disp->Driver->QueryDriverConfig(disp);
      }
      static const char *EGLAPIENTRY
   eglGetDisplayDriverName(EGLDisplay dpy)
   {
      _EGLDisplay *disp = _eglLockDisplay(dpy);
            _EGL_FUNC_START(disp, EGL_NONE, NULL);
                     ret = disp->Driver->QueryDriverName(disp);
      }
      PUBLIC __eglMustCastToProperFunctionPointerType EGLAPIENTRY
   eglGetProcAddress(const char *procname)
   {
         #define EGL_ENTRYPOINT(f)     {.name = #f, .function = (_EGLProc)f},
   #define EGL_ENTRYPOINT2(n, f) {.name = #n, .function = (_EGLProc)f},
   #include "eglentrypoint.h"
   #undef EGL_ENTRYPOINT2
   #undef EGL_ENTRYPOINT
      };
            if (!procname)
                     if (strncmp(procname, "egl", 3) == 0) {
      const struct _egl_entrypoint *entrypoint =
      bsearch(procname, egl_functions, ARRAY_SIZE(egl_functions),
      if (entrypoint)
               if (!ret)
               }
      static int
   _eglLockDisplayInterop(EGLDisplay dpy, EGLContext context, _EGLDisplay **disp,
         {
         *disp = _eglLockDisplay(dpy);
   if (!*disp || !(*disp)->Initialized || !(*disp)->Driver) {
      if (*disp)
                     *ctx = _eglLookupContext(context, *disp);
   if (!*ctx) {
      _eglUnlockDisplay(*disp);
                  }
      PUBLIC int
   MesaGLInteropEGLQueryDeviceInfo(EGLDisplay dpy, EGLContext context,
         {
      _EGLDisplay *disp;
   _EGLContext *ctx;
            ret = _eglLockDisplayInterop(dpy, context, &disp, &ctx);
   if (ret != MESA_GLINTEROP_SUCCESS)
            if (disp->Driver->GLInteropQueryDeviceInfo)
         else
            _eglUnlockDisplay(disp);
      }
      PUBLIC int
   MesaGLInteropEGLExportObject(EGLDisplay dpy, EGLContext context,
               {
      _EGLDisplay *disp;
   _EGLContext *ctx;
            ret = _eglLockDisplayInterop(dpy, context, &disp, &ctx);
   if (ret != MESA_GLINTEROP_SUCCESS)
            if (disp->Driver->GLInteropExportObject)
         else
            _eglUnlockDisplay(disp);
      }
      PUBLIC int
   MesaGLInteropEGLFlushObjects(EGLDisplay dpy, EGLContext context, unsigned count,
               {
      _EGLDisplay *disp;
   _EGLContext *ctx;
            ret = _eglLockDisplayInterop(dpy, context, &disp, &ctx);
   if (ret != MESA_GLINTEROP_SUCCESS)
            if (disp->Driver->GLInteropFlushObjects)
      ret =
      else
            _eglUnlockDisplay(disp);
      }
