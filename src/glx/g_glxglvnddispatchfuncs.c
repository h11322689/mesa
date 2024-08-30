   #include <stdlib.h>
      #include "glxclient.h"
   #include "glxglvnd.h"
   #include "glxglvnddispatchfuncs.h"
   #include "g_glxglvnddispatchindices.h"
      #include "GL/mesa_glinterop.h"
      const int DI_FUNCTION_COUNT = DI_LAST_INDEX;
   /* Allocate an extra 'dummy' to ease lookup. See FindGLXFunction() */
   int __glXDispatchTableIndices[DI_LAST_INDEX + 1];
   const __GLXapiExports *__glXGLVNDAPIExports;
      const char * const __glXDispatchTableStrings[DI_LAST_INDEX] = {
   #define __ATTRIB(field) \
               __ATTRIB(BindSwapBarrierSGIX),
   __ATTRIB(BindTexImageEXT),
   // glXChooseFBConfig implemented by libglvnd
   __ATTRIB(ChooseFBConfigSGIX),
   // glXChooseVisual implemented by libglvnd
   // glXCopyContext implemented by libglvnd
   __ATTRIB(CopySubBufferMESA),
   // glXCreateContext implemented by libglvnd
   __ATTRIB(CreateContextAttribsARB),
   __ATTRIB(CreateContextWithConfigSGIX),
   __ATTRIB(CreateGLXPbufferSGIX),
   // glXCreateGLXPixmap implemented by libglvnd
   __ATTRIB(CreateGLXPixmapMESA),
   __ATTRIB(CreateGLXPixmapWithConfigSGIX),
   // glXCreateNewContext implemented by libglvnd
   // glXCreatePbuffer implemented by libglvnd
   // glXCreatePixmap implemented by libglvnd
   // glXCreateWindow implemented by libglvnd
   // glXDestroyContext implemented by libglvnd
   __ATTRIB(DestroyGLXPbufferSGIX),
   // glXDestroyGLXPixmap implemented by libglvnd
   // glXDestroyPbuffer implemented by libglvnd
   // glXDestroyPixmap implemented by libglvnd
   // glXDestroyWindow implemented by libglvnd
   // glXFreeContextEXT implemented by libglvnd
   __ATTRIB(GLInteropExportObjectMESA),
   __ATTRIB(GLInteropFlushObjectsMESA),
   __ATTRIB(GLInteropQueryDeviceInfoMESA),
   // glXGetClientString implemented by libglvnd
   // glXGetConfig implemented by libglvnd
   __ATTRIB(GetContextIDEXT),
   // glXGetCurrentContext implemented by libglvnd
   // glXGetCurrentDisplay implemented by libglvnd
   __ATTRIB(GetCurrentDisplayEXT),
   // glXGetCurrentDrawable implemented by libglvnd
   // glXGetCurrentReadDrawable implemented by libglvnd
   __ATTRIB(GetDriverConfig),
   // glXGetFBConfigAttrib implemented by libglvnd
   __ATTRIB(GetFBConfigAttribSGIX),
   __ATTRIB(GetFBConfigFromVisualSGIX),
   // glXGetFBConfigs implemented by libglvnd
   __ATTRIB(GetMscRateOML),
   // glXGetProcAddress implemented by libglvnd
   // glXGetProcAddressARB implemented by libglvnd
   __ATTRIB(GetScreenDriver),
   // glXGetSelectedEvent implemented by libglvnd
   __ATTRIB(GetSelectedEventSGIX),
   __ATTRIB(GetSwapIntervalMESA),
   __ATTRIB(GetSyncValuesOML),
   __ATTRIB(GetVideoSyncSGI),
   // glXGetVisualFromFBConfig implemented by libglvnd
   __ATTRIB(GetVisualFromFBConfigSGIX),
   // glXImportContextEXT implemented by libglvnd
   // glXIsDirect implemented by libglvnd
   __ATTRIB(JoinSwapGroupSGIX),
   // glXMakeContextCurrent implemented by libglvnd
   // glXMakeCurrent implemented by libglvnd
   // glXQueryContext implemented by libglvnd
   __ATTRIB(QueryContextInfoEXT),
   __ATTRIB(QueryCurrentRendererIntegerMESA),
   __ATTRIB(QueryCurrentRendererStringMESA),
   // glXQueryDrawable implemented by libglvnd
   // glXQueryExtension implemented by libglvnd
   // glXQueryExtensionsString implemented by libglvnd
   __ATTRIB(QueryGLXPbufferSGIX),
   __ATTRIB(QueryMaxSwapBarriersSGIX),
   __ATTRIB(QueryRendererIntegerMESA),
   __ATTRIB(QueryRendererStringMESA),
   // glXQueryServerString implemented by libglvnd
   // glXQueryVersion implemented by libglvnd
   __ATTRIB(ReleaseBuffersMESA),
   __ATTRIB(ReleaseTexImageEXT),
   // glXSelectEvent implemented by libglvnd
   __ATTRIB(SelectEventSGIX),
   // glXSwapBuffers implemented by libglvnd
   __ATTRIB(SwapBuffersMscOML),
   __ATTRIB(SwapIntervalEXT),
   __ATTRIB(SwapIntervalMESA),
   __ATTRIB(SwapIntervalSGI),
   // glXUseXFont implemented by libglvnd
   __ATTRIB(WaitForMscOML),
   __ATTRIB(WaitForSbcOML),
   // glXWaitGL implemented by libglvnd
   __ATTRIB(WaitVideoSyncSGI),
         #undef __ATTRIB
   };
      #define __FETCH_FUNCTION_PTR(func_name) \
      p##func_name = (void *) \
            static void dispatch_BindTexImageEXT(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXBINDTEXIMAGEEXTPROC pBindTexImageEXT;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(BindTexImageEXT);
   if (pBindTexImageEXT == NULL)
               }
            static GLXFBConfigSGIX *dispatch_ChooseFBConfigSGIX(Display *dpy, int screen,
               {
      PFNGLXCHOOSEFBCONFIGSGIXPROC pChooseFBConfigSGIX;
   __GLXvendorInfo *dd;
            dd = __VND->getDynDispatch(dpy, screen);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(ChooseFBConfigSGIX);
   if (pChooseFBConfigSGIX == NULL)
            ret = pChooseFBConfigSGIX(dpy, screen, attrib_list, nelements);
   if (AddFBConfigsMapping(dpy, ret, nelements, dd)) {
      free(ret);
                  }
            static GLXContext dispatch_CreateContextAttribsARB(Display *dpy,
                           {
      PFNGLXCREATECONTEXTATTRIBSARBPROC pCreateContextAttribsARB;
   __GLXvendorInfo *dd = NULL;
            if (config) {
         } else if (attrib_list) {
               for (i = 0; attrib_list[i * 2] != None; i++) {
      if (attrib_list[i * 2] == GLX_SCREEN) {
      screen = attrib_list[i * 2 + 1];
   dd = GetDispatchFromDrawable(dpy, RootWindow(dpy, screen));
            }
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(CreateContextAttribsARB);
   if (pCreateContextAttribsARB == NULL)
            ret = pCreateContextAttribsARB(dpy, config, share_list, direct, attrib_list);
   if (AddContextMapping(dpy, ret, dd)) {
      /* XXX: Call glXDestroyContext which lives in libglvnd. If we're not
      * allowed to call it from here, should we extend __glXDispatchTableIndices ?
                     }
            static GLXContext dispatch_CreateContextWithConfigSGIX(Display *dpy,
                           {
      PFNGLXCREATECONTEXTWITHCONFIGSGIXPROC pCreateContextWithConfigSGIX;
   __GLXvendorInfo *dd;
            dd = GetDispatchFromFBConfig(dpy, config);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(CreateContextWithConfigSGIX);
   if (pCreateContextWithConfigSGIX == NULL)
            ret = pCreateContextWithConfigSGIX(dpy, config, render_type, share_list, direct);
   if (AddContextMapping(dpy, ret, dd)) {
      /* XXX: Call glXDestroyContext which lives in libglvnd. If we're not
      * allowed to call it from here, should we extend __glXDispatchTableIndices ?
                     }
            static GLXPbuffer dispatch_CreateGLXPbufferSGIX(Display *dpy,
                           {
      PFNGLXCREATEGLXPBUFFERSGIXPROC pCreateGLXPbufferSGIX;
   __GLXvendorInfo *dd;
            dd = GetDispatchFromFBConfig(dpy, config);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(CreateGLXPbufferSGIX);
   if (pCreateGLXPbufferSGIX == NULL)
            ret = pCreateGLXPbufferSGIX(dpy, config, width, height, attrib_list);
   if (AddDrawableMapping(dpy, ret, dd)) {
               __FETCH_FUNCTION_PTR(DestroyGLXPbufferSGIX);
   if (pDestroyGLXPbufferSGIX)
                           }
            static GLXPixmap dispatch_CreateGLXPixmapWithConfigSGIX(Display *dpy,
               {
      PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIXPROC pCreateGLXPixmapWithConfigSGIX;
   __GLXvendorInfo *dd;
            dd = GetDispatchFromFBConfig(dpy, config);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(CreateGLXPixmapWithConfigSGIX);
   if (pCreateGLXPixmapWithConfigSGIX == NULL)
            ret = pCreateGLXPixmapWithConfigSGIX(dpy, config, pixmap);
   if (AddDrawableMapping(dpy, ret, dd)) {
      /* XXX: Call glXDestroyGLXPixmap which lives in libglvnd. If we're not
      * allowed to call it from here, should we extend __glXDispatchTableIndices ?
                     }
            static void dispatch_DestroyGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf)
   {
      PFNGLXDESTROYGLXPBUFFERSGIXPROC pDestroyGLXPbufferSGIX;
            dd = GetDispatchFromDrawable(dpy, pbuf);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(DestroyGLXPbufferSGIX);
   if (pDestroyGLXPbufferSGIX == NULL)
               }
            static int dispatch_GLInteropExportObjectMESA(Display *dpy, GLXContext ctx,
               {
      PFNMESAGLINTEROPGLXEXPORTOBJECTPROC pGLInteropExportObjectMESA;
            dd = GetDispatchFromContext(ctx);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GLInteropExportObjectMESA);
   if (pGLInteropExportObjectMESA == NULL)
               }
         static int dispatch_GLInteropFlushObjectsMESA(Display *dpy, GLXContext ctx,
                     {
      PFNMESAGLINTEROPGLXFLUSHOBJECTSPROC pGLInteropFlushObjectsMESA;
            dd = GetDispatchFromContext(ctx);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GLInteropFlushObjectsMESA);
   if (pGLInteropFlushObjectsMESA == NULL)
               }
         static int dispatch_GLInteropQueryDeviceInfoMESA(Display *dpy, GLXContext ctx,
         {
      PFNMESAGLINTEROPGLXQUERYDEVICEINFOPROC pGLInteropQueryDeviceInfoMESA;
            dd = GetDispatchFromContext(ctx);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GLInteropQueryDeviceInfoMESA);
   if (pGLInteropQueryDeviceInfoMESA == NULL)
               }
         static GLXContextID dispatch_GetContextIDEXT(const GLXContext ctx)
   {
      PFNGLXGETCONTEXTIDEXTPROC pGetContextIDEXT;
            dd = GetDispatchFromContext(ctx);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetContextIDEXT);
   if (pGetContextIDEXT == NULL)
               }
            static Display *dispatch_GetCurrentDisplayEXT(void)
   {
      PFNGLXGETCURRENTDISPLAYEXTPROC pGetCurrentDisplayEXT;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetCurrentDisplayEXT);
   if (pGetCurrentDisplayEXT == NULL)
               }
            static const char *dispatch_GetDriverConfig(const char *driverName)
   {
   #if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
      /*
   * The options are constant for a given driverName, so we do not need
   * a context (and apps expect to be able to call this without one).
   */
      #else
         #endif
   }
            static int dispatch_GetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config,
         {
      PFNGLXGETFBCONFIGATTRIBSGIXPROC pGetFBConfigAttribSGIX;
            dd = GetDispatchFromFBConfig(dpy, config);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetFBConfigAttribSGIX);
   if (pGetFBConfigAttribSGIX == NULL)
               }
            static GLXFBConfigSGIX dispatch_GetFBConfigFromVisualSGIX(Display *dpy,
         {
      PFNGLXGETFBCONFIGFROMVISUALSGIXPROC pGetFBConfigFromVisualSGIX;
   __GLXvendorInfo *dd;
            dd = GetDispatchFromVisual(dpy, vis);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetFBConfigFromVisualSGIX);
   if (pGetFBConfigFromVisualSGIX == NULL)
            ret = pGetFBConfigFromVisualSGIX(dpy, vis);
   if (AddFBConfigMapping(dpy, ret, dd))
      /* XXX: dealloc ret ? */
            }
            static void dispatch_GetSelectedEventSGIX(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXGETSELECTEDEVENTSGIXPROC pGetSelectedEventSGIX;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetSelectedEventSGIX);
   if (pGetSelectedEventSGIX == NULL)
               }
            static int dispatch_GetVideoSyncSGI(unsigned int *count)
   {
      PFNGLXGETVIDEOSYNCSGIPROC pGetVideoSyncSGI;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetVideoSyncSGI);
   if (pGetVideoSyncSGI == NULL)
               }
            static XVisualInfo *dispatch_GetVisualFromFBConfigSGIX(Display *dpy,
         {
      PFNGLXGETVISUALFROMFBCONFIGSGIXPROC pGetVisualFromFBConfigSGIX;
            dd = GetDispatchFromFBConfig(dpy, config);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetVisualFromFBConfigSGIX);
   if (pGetVisualFromFBConfigSGIX == NULL)
               }
            static int dispatch_QueryContextInfoEXT(Display *dpy, GLXContext ctx,
         {
      PFNGLXQUERYCONTEXTINFOEXTPROC pQueryContextInfoEXT;
            dd = GetDispatchFromContext(ctx);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryContextInfoEXT);
   if (pQueryContextInfoEXT == NULL)
               }
            static void dispatch_QueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf,
         {
      PFNGLXQUERYGLXPBUFFERSGIXPROC pQueryGLXPbufferSGIX;
            dd = GetDispatchFromDrawable(dpy, pbuf);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryGLXPbufferSGIX);
   if (pQueryGLXPbufferSGIX == NULL)
               }
            static void dispatch_ReleaseTexImageEXT(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXRELEASETEXIMAGEEXTPROC pReleaseTexImageEXT;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(ReleaseTexImageEXT);
   if (pReleaseTexImageEXT == NULL)
               }
            static void dispatch_SelectEventSGIX(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXSELECTEVENTSGIXPROC pSelectEventSGIX;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(SelectEventSGIX);
   if (pSelectEventSGIX == NULL)
               }
            static int dispatch_SwapIntervalSGI(int interval)
   {
      PFNGLXSWAPINTERVALSGIPROC pSwapIntervalSGI;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(SwapIntervalSGI);
   if (pSwapIntervalSGI == NULL)
               }
            static int dispatch_WaitVideoSyncSGI(int divisor, int remainder,
         {
      PFNGLXWAITVIDEOSYNCSGIPROC pWaitVideoSyncSGI;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(WaitVideoSyncSGI);
   if (pWaitVideoSyncSGI == NULL)
               }
            static void dispatch_BindSwapBarrierSGIX(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXBINDSWAPBARRIERSGIXPROC pBindSwapBarrierSGIX;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(BindSwapBarrierSGIX);
   if (pBindSwapBarrierSGIX == NULL)
               }
            static void dispatch_CopySubBufferMESA(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXCOPYSUBBUFFERMESAPROC pCopySubBufferMESA;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(CopySubBufferMESA);
   if (pCopySubBufferMESA == NULL)
               }
            static GLXPixmap dispatch_CreateGLXPixmapMESA(Display *dpy,
               {
      PFNGLXCREATEGLXPIXMAPMESAPROC pCreateGLXPixmapMESA;
   __GLXvendorInfo *dd;
            dd = GetDispatchFromVisual(dpy, visinfo);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(CreateGLXPixmapMESA);
   if (pCreateGLXPixmapMESA == NULL)
            ret = pCreateGLXPixmapMESA(dpy, visinfo, pixmap, cmap);
   if (AddDrawableMapping(dpy, ret, dd)) {
      /* XXX: Call glXDestroyGLXPixmap which lives in libglvnd. If we're not
      * allowed to call it from here, should we extend __glXDispatchTableIndices ?
                     }
            static GLboolean dispatch_GetMscRateOML(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXGETMSCRATEOMLPROC pGetMscRateOML;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetMscRateOML);
   if (pGetMscRateOML == NULL)
               }
            static const char *dispatch_GetScreenDriver(Display *dpy, int scrNum)
   {
      typedef const char *(*fn_glXGetScreenDriver_ptr)(Display *dpy, int scrNum);
   fn_glXGetScreenDriver_ptr pGetScreenDriver;
            dd = __VND->getDynDispatch(dpy, scrNum);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetScreenDriver);
   if (pGetScreenDriver == NULL)
               }
            static int dispatch_GetSwapIntervalMESA(void)
   {
      PFNGLXGETSWAPINTERVALMESAPROC pGetSwapIntervalMESA;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetSwapIntervalMESA);
   if (pGetSwapIntervalMESA == NULL)
               }
            static Bool dispatch_GetSyncValuesOML(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXGETSYNCVALUESOMLPROC pGetSyncValuesOML;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(GetSyncValuesOML);
   if (pGetSyncValuesOML == NULL)
               }
            static void dispatch_JoinSwapGroupSGIX(Display *dpy, GLXDrawable drawable,
         {
      PFNGLXJOINSWAPGROUPSGIXPROC pJoinSwapGroupSGIX;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(JoinSwapGroupSGIX);
   if (pJoinSwapGroupSGIX == NULL)
               }
            static Bool dispatch_QueryCurrentRendererIntegerMESA(int attribute,
         {
      PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC pQueryCurrentRendererIntegerMESA;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryCurrentRendererIntegerMESA);
   if (pQueryCurrentRendererIntegerMESA == NULL)
               }
            static const char *dispatch_QueryCurrentRendererStringMESA(int attribute)
   {
      PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC pQueryCurrentRendererStringMESA;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryCurrentRendererStringMESA);
   if (pQueryCurrentRendererStringMESA == NULL)
               }
            static Bool dispatch_QueryMaxSwapBarriersSGIX(Display *dpy, int screen,
         {
      PFNGLXQUERYMAXSWAPBARRIERSSGIXPROC pQueryMaxSwapBarriersSGIX;
            dd = __VND->getDynDispatch(dpy, screen);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryMaxSwapBarriersSGIX);
   if (pQueryMaxSwapBarriersSGIX == NULL)
               }
            static Bool dispatch_QueryRendererIntegerMESA(Display *dpy, int screen,
               {
      PFNGLXQUERYRENDERERINTEGERMESAPROC pQueryRendererIntegerMESA;
            dd = __VND->getDynDispatch(dpy, screen);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryRendererIntegerMESA);
   if (pQueryRendererIntegerMESA == NULL)
               }
            static const char *dispatch_QueryRendererStringMESA(Display *dpy, int screen,
         {
      PFNGLXQUERYRENDERERSTRINGMESAPROC pQueryRendererStringMESA;
            dd = __VND->getDynDispatch(dpy, screen);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(QueryRendererStringMESA);
   if (pQueryRendererStringMESA == NULL)
               }
            static Bool dispatch_ReleaseBuffersMESA(Display *dpy, GLXDrawable d)
   {
      PFNGLXRELEASEBUFFERSMESAPROC pReleaseBuffersMESA;
            dd = GetDispatchFromDrawable(dpy, d);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(ReleaseBuffersMESA);
   if (pReleaseBuffersMESA == NULL)
               }
            static int64_t dispatch_SwapBuffersMscOML(Display *dpy, GLXDrawable drawable,
               {
      PFNGLXSWAPBUFFERSMSCOMLPROC pSwapBuffersMscOML;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(SwapBuffersMscOML);
   if (pSwapBuffersMscOML == NULL)
               }
            static int dispatch_SwapIntervalMESA(unsigned int interval)
   {
      PFNGLXSWAPINTERVALMESAPROC pSwapIntervalMESA;
            if (!__VND->getCurrentContext())
            dd = __VND->getCurrentDynDispatch();
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(SwapIntervalMESA);
   if (pSwapIntervalMESA == NULL)
               }
            static void dispatch_SwapIntervalEXT(Display *dpy, GLXDrawable drawable, int interval)
   {
      PFNGLXSWAPINTERVALEXTPROC pSwapIntervalEXT;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(SwapIntervalEXT);
   if (pSwapIntervalEXT == NULL)
               }
            static Bool dispatch_WaitForMscOML(Display *dpy, GLXDrawable drawable,
                     {
      PFNGLXWAITFORMSCOMLPROC pWaitForMscOML;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(WaitForMscOML);
   if (pWaitForMscOML == NULL)
               }
            static Bool dispatch_WaitForSbcOML(Display *dpy, GLXDrawable drawable,
               {
      PFNGLXWAITFORSBCOMLPROC pWaitForSbcOML;
            dd = GetDispatchFromDrawable(dpy, drawable);
   if (dd == NULL)
            __FETCH_FUNCTION_PTR(WaitForSbcOML);
   if (pWaitForSbcOML == NULL)
               }
      #undef __FETCH_FUNCTION_PTR
         /* Allocate an extra 'dummy' to ease lookup. See FindGLXFunction() */
   const void * const __glXDispatchFunctions[DI_LAST_INDEX + 1] = {
   #define __ATTRIB(field) \
               __ATTRIB(BindSwapBarrierSGIX),
   __ATTRIB(BindTexImageEXT),
   __ATTRIB(ChooseFBConfigSGIX),
   __ATTRIB(CopySubBufferMESA),
   __ATTRIB(CreateContextAttribsARB),
   __ATTRIB(CreateContextWithConfigSGIX),
   __ATTRIB(CreateGLXPbufferSGIX),
   __ATTRIB(CreateGLXPixmapMESA),
   __ATTRIB(CreateGLXPixmapWithConfigSGIX),
   __ATTRIB(DestroyGLXPbufferSGIX),
   __ATTRIB(GLInteropExportObjectMESA),
   __ATTRIB(GLInteropFlushObjectsMESA),
   __ATTRIB(GLInteropQueryDeviceInfoMESA),
   __ATTRIB(GetContextIDEXT),
   __ATTRIB(GetCurrentDisplayEXT),
   __ATTRIB(GetDriverConfig),
   __ATTRIB(GetFBConfigAttribSGIX),
   __ATTRIB(GetFBConfigFromVisualSGIX),
   __ATTRIB(GetMscRateOML),
   __ATTRIB(GetScreenDriver),
   __ATTRIB(GetSelectedEventSGIX),
   __ATTRIB(GetSwapIntervalMESA),
   __ATTRIB(GetSyncValuesOML),
   __ATTRIB(GetVideoSyncSGI),
   __ATTRIB(GetVisualFromFBConfigSGIX),
   __ATTRIB(JoinSwapGroupSGIX),
   __ATTRIB(QueryContextInfoEXT),
   __ATTRIB(QueryCurrentRendererIntegerMESA),
   __ATTRIB(QueryCurrentRendererStringMESA),
   __ATTRIB(QueryGLXPbufferSGIX),
   __ATTRIB(QueryMaxSwapBarriersSGIX),
   __ATTRIB(QueryRendererIntegerMESA),
   __ATTRIB(QueryRendererStringMESA),
   __ATTRIB(ReleaseBuffersMESA),
   __ATTRIB(ReleaseTexImageEXT),
   __ATTRIB(SelectEventSGIX),
   __ATTRIB(SwapBuffersMscOML),
   __ATTRIB(SwapIntervalEXT),
   __ATTRIB(SwapIntervalMESA),
   __ATTRIB(SwapIntervalSGI),
   __ATTRIB(WaitForMscOML),
   __ATTRIB(WaitForSbcOML),
               #undef __ATTRIB
   };
