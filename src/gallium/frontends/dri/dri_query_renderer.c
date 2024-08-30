   #include "dri_query_renderer.h"
      #include "util/u_inlines.h"
   #include "frontend/drm_driver.h"
      #include "dri_screen.h"
   #include "dri_query_renderer.h"
   #include "pipe-loader/pipe_loader.h"
      /**
   * Implement queries for values that are common across all Mesa drivers
   *
   * Currently only the following queries are supported by this function:
   *
   *     - \c __DRI2_RENDERER_VERSION
   *     - \c __DRI2_RENDERER_PREFERRED_PROFILE
   *     - \c __DRI2_RENDERER_OPENGL_CORE_PROFILE_VERSION
   *     - \c __DRI2_RENDERER_OPENGL_COMPATIBLITY_PROFILE_VERSION
   *     - \c __DRI2_RENDERER_ES_PROFILE_VERSION
   *     - \c __DRI2_RENDERER_ES2_PROFILE_VERSION
   *
   * \returns
   * Zero if a recognized value of \c param is supplied, -1 otherwise.
   */
   static int
   driQueryRendererIntegerCommon(struct dri_screen *screen, int param, unsigned int *value)
   {
      switch (param) {
   case __DRI2_RENDERER_VERSION: {
      static const char *const ver = PACKAGE_VERSION;
   char *endptr;
            v[0] = strtol(ver, &endptr, 10);
   assert(endptr[0] == '.');
   if (endptr[0] != '.')
            v[1] = strtol(endptr + 1, &endptr, 10);
   assert(endptr[0] == '.');
   if (endptr[0] != '.')
                     value[0] = v[0];
   value[1] = v[1];
   value[2] = v[2];
      }
   case __DRI2_RENDERER_PREFERRED_PROFILE:
      value[0] = (screen->max_gl_core_version != 0)
            case __DRI2_RENDERER_OPENGL_CORE_PROFILE_VERSION:
      value[0] = screen->max_gl_core_version / 10;
   value[1] = screen->max_gl_core_version % 10;
      case __DRI2_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION:
      value[0] = screen->max_gl_compat_version / 10;
   value[1] = screen->max_gl_compat_version % 10;
      case __DRI2_RENDERER_OPENGL_ES_PROFILE_VERSION:
      value[0] = screen->max_gl_es1_version / 10;
   value[1] = screen->max_gl_es1_version % 10;
      case __DRI2_RENDERER_OPENGL_ES2_PROFILE_VERSION:
      value[0] = screen->max_gl_es2_version / 10;
   value[1] = screen->max_gl_es2_version % 10;
      default:
                     }
      static int
   dri2_query_renderer_integer(__DRIscreen *_screen, int param,
         {
               switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] =
      (unsigned int)screen->base.screen->get_param(screen->base.screen,
         case __DRI2_RENDERER_DEVICE_ID:
      value[0] =
      (unsigned int)screen->base.screen->get_param(screen->base.screen,
         case __DRI2_RENDERER_ACCELERATED:
      value[0] =
      (unsigned int)!!screen->base.screen->get_param(screen->base.screen,
            case __DRI2_RENDERER_VIDEO_MEMORY: {
      int ov = driQueryOptioni(&screen->dev->option_cache, "override_vram_size");
   value[0] =
      (unsigned int)screen->base.screen->get_param(screen->base.screen,
      if (ov >= 0)
                     case __DRI2_RENDERER_UNIFIED_MEMORY_ARCHITECTURE:
      value[0] =
      (unsigned int)screen->base.screen->get_param(screen->base.screen,
            case __DRI2_RENDERER_PREFER_BACK_BUFFER_REUSE:
      value[0] =
      screen->base.screen->get_param(screen->base.screen,
         default:
            }
      static int
   dri2_query_renderer_string(__DRIscreen *_screen, int param,
         {
               switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] = screen->base.screen->get_vendor(screen->base.screen);
      case __DRI2_RENDERER_DEVICE_ID:
      value[0] = screen->base.screen->get_name(screen->base.screen);
      default:
            }
      const __DRI2rendererQueryExtension dri2RendererQueryExtension = {
               .queryInteger         = dri2_query_renderer_integer,
      };
