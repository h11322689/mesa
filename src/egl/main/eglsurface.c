   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
   * Copyright 2010 LunarG, Inc.
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
   * Surface-related functions.
   */
      #include "eglsurface.h"
   #include <assert.h>
   #include <stdlib.h>
   #include <string.h>
   #include "eglconfig.h"
   #include "eglcontext.h"
   #include "eglcurrent.h"
   #include "egldefines.h"
   #include "egldisplay.h"
   #include "egldriver.h"
   #include "egllog.h"
      #include "util/macros.h"
      /**
   * Parse the list of surface attributes and return the proper error code.
   */
   static EGLint
   _eglParseSurfaceAttribList(_EGLSurface *surf, const EGLint *attrib_list)
   {
      _EGLDisplay *disp = surf->Resource.Display;
   EGLint type = surf->Type;
   EGLint texture_type = EGL_PBUFFER_BIT;
   EGLint i, err = EGL_SUCCESS;
   EGLint attr = EGL_NONE;
            if (!attrib_list)
            if (disp->Extensions.NOK_texture_from_pixmap)
            for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      attr = attrib_list[i++];
            switch (attr) {
   /* common attributes */
   case EGL_GL_COLORSPACE_KHR:
      if (!disp->Extensions.KHR_gl_colorspace) {
      err = EGL_BAD_ATTRIBUTE;
      }
   switch (val) {
   case EGL_GL_COLORSPACE_SRGB_KHR:
   case EGL_GL_COLORSPACE_LINEAR_KHR:
         default:
         }
   if (err != EGL_SUCCESS)
         surf->GLColorspace = val;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_RX_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.display_primary_r.x = val;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_RY_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.display_primary_r.y = val;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_GX_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.display_primary_g.x = val;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_GY_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.display_primary_g.y = val;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_BX_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.display_primary_b.x = val;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_BY_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.display_primary_b.y = val;
      case EGL_SMPTE2086_WHITE_POINT_X_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.white_point.x = val;
      case EGL_SMPTE2086_WHITE_POINT_Y_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.white_point.y = val;
      case EGL_SMPTE2086_MAX_LUMINANCE_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.max_luminance = val;
      case EGL_SMPTE2086_MIN_LUMINANCE_EXT:
      if (!disp->Extensions.EXT_surface_SMPTE2086_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.min_luminance = val;
      case EGL_CTA861_3_MAX_CONTENT_LIGHT_LEVEL_EXT:
      if (!disp->Extensions.EXT_surface_CTA861_3_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.max_cll = val;
      case EGL_CTA861_3_MAX_FRAME_AVERAGE_LEVEL_EXT:
      if (!disp->Extensions.EXT_surface_CTA861_3_metadata) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->HdrMetadata.max_fall = val;
      case EGL_VG_COLORSPACE:
      switch (val) {
   case EGL_VG_COLORSPACE_sRGB:
   case EGL_VG_COLORSPACE_LINEAR:
         default:
      err = EGL_BAD_ATTRIBUTE;
      }
   if (err != EGL_SUCCESS)
         surf->VGColorspace = val;
      case EGL_VG_ALPHA_FORMAT:
      switch (val) {
   case EGL_VG_ALPHA_FORMAT_NONPRE:
   case EGL_VG_ALPHA_FORMAT_PRE:
         default:
      err = EGL_BAD_ATTRIBUTE;
      }
   if (err != EGL_SUCCESS)
         surf->VGAlphaFormat = val;
      /* window surface attributes */
   case EGL_RENDER_BUFFER:
      if (type != EGL_WINDOW_BIT) {
      err = EGL_BAD_ATTRIBUTE;
      }
   if (val != EGL_BACK_BUFFER && val != EGL_SINGLE_BUFFER) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->RequestedRenderBuffer = val;
   if (surf->Config->SurfaceType & EGL_MUTABLE_RENDER_BUFFER_BIT_KHR) {
      /* Unlike normal EGLSurfaces, one with a mutable render buffer
   * uses the application-chosen render buffer.
   */
      }
      case EGL_PRESENT_OPAQUE_EXT:
      if (!disp->Extensions.EXT_present_opaque) {
      err = EGL_BAD_ATTRIBUTE;
      }
   if (type != EGL_WINDOW_BIT) {
      err = EGL_BAD_ATTRIBUTE;
      }
   if (val != EGL_TRUE && val != EGL_FALSE) {
      err = EGL_BAD_PARAMETER;
      }
   surf->PresentOpaque = val;
      case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
      if (!disp->Extensions.NV_post_sub_buffer || type != EGL_WINDOW_BIT) {
      err = EGL_BAD_ATTRIBUTE;
      }
   if (val != EGL_TRUE && val != EGL_FALSE) {
      err = EGL_BAD_PARAMETER;
      }
   surf->PostSubBufferSupportedNV = val;
      /* pbuffer surface attributes */
   case EGL_WIDTH:
      if (type != EGL_PBUFFER_BIT) {
      err = EGL_BAD_ATTRIBUTE;
      }
   if (val < 0) {
      err = EGL_BAD_PARAMETER;
      }
   surf->Width = val;
      case EGL_HEIGHT:
      if (type != EGL_PBUFFER_BIT) {
      err = EGL_BAD_ATTRIBUTE;
      }
   if (val < 0) {
      err = EGL_BAD_PARAMETER;
      }
   surf->Height = val;
      case EGL_LARGEST_PBUFFER:
      if (type != EGL_PBUFFER_BIT) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->LargestPbuffer = !!val;
      /* for eglBindTexImage */
   case EGL_TEXTURE_FORMAT:
      if (!(type & texture_type)) {
      err = EGL_BAD_ATTRIBUTE;
               switch (val) {
   case EGL_TEXTURE_RGB:
   case EGL_TEXTURE_RGBA:
   case EGL_NO_TEXTURE:
         default:
      err = EGL_BAD_ATTRIBUTE;
      }
   if (err != EGL_SUCCESS)
         surf->TextureFormat = val;
      case EGL_TEXTURE_TARGET:
      if (!(type & texture_type)) {
      err = EGL_BAD_ATTRIBUTE;
               switch (val) {
   case EGL_TEXTURE_2D:
   case EGL_NO_TEXTURE:
         default:
      err = EGL_BAD_ATTRIBUTE;
      }
   if (err != EGL_SUCCESS)
         surf->TextureTarget = val;
      case EGL_MIPMAP_TEXTURE:
      if (!(type & texture_type)) {
      err = EGL_BAD_ATTRIBUTE;
      }
   surf->MipmapTexture = !!val;
      case EGL_PROTECTED_CONTENT_EXT:
      if (!disp->Extensions.EXT_protected_content &&
      !disp->Extensions.EXT_protected_surface) {
   err = EGL_BAD_ATTRIBUTE;
      }
               /* no pixmap surface specific attributes */
   default:
      err = EGL_BAD_ATTRIBUTE;
               if (err != EGL_SUCCESS)
               if (err == EGL_SUCCESS && type == EGL_PBUFFER_BIT) {
      if ((surf->TextureTarget == EGL_NO_TEXTURE &&
      surf->TextureFormat != EGL_NO_TEXTURE) ||
   (surf->TextureFormat == EGL_NO_TEXTURE &&
   surf->TextureTarget != EGL_NO_TEXTURE)) {
   attr = surf->TextureTarget == EGL_NO_TEXTURE ? EGL_TEXTURE_TARGET
                        if (err != EGL_SUCCESS)
               }
      /**
   * Do error check on parameters and initialize the given _EGLSurface object.
   * \return EGL_TRUE if no errors, EGL_FALSE otherwise.
   */
   EGLBoolean
   _eglInitSurface(_EGLSurface *surf, _EGLDisplay *disp, EGLint type,
               {
      const char *func;
   EGLint renderBuffer = EGL_BACK_BUFFER;
   EGLint swapBehavior = EGL_BUFFER_DESTROYED;
            /* Swap behavior can be preserved only if config supports this. */
   if (conf->SurfaceType & EGL_SWAP_BEHAVIOR_PRESERVED_BIT)
            switch (type) {
   case EGL_WINDOW_BIT:
      func = "eglCreateWindowSurface";
   swapBehavior = EGL_BUFFER_DESTROYED;
      case EGL_PIXMAP_BIT:
      func = "eglCreatePixmapSurface";
   renderBuffer = EGL_SINGLE_BUFFER;
      case EGL_PBUFFER_BIT:
      func = "eglCreatePBufferSurface";
      default:
      _eglLog(_EGL_WARNING, "Bad type in _eglInitSurface");
               if ((conf->SurfaceType & type) == 0)
      /* The config can't be used to create a surface of this type */
         _eglInitResource(&surf->Resource, sizeof(*surf), disp);
   surf->Type = type;
   surf->Config = conf;
            surf->Width = 0;
   surf->Height = 0;
   surf->TextureFormat = EGL_NO_TEXTURE;
   surf->TextureTarget = EGL_NO_TEXTURE;
   surf->MipmapTexture = EGL_FALSE;
   surf->LargestPbuffer = EGL_FALSE;
   surf->RequestedRenderBuffer = renderBuffer;
   surf->ActiveRenderBuffer = renderBuffer;
   surf->VGAlphaFormat = EGL_VG_ALPHA_FORMAT_NONPRE;
   surf->VGColorspace = EGL_VG_COLORSPACE_sRGB;
   surf->GLColorspace = EGL_GL_COLORSPACE_LINEAR_KHR;
   surf->ProtectedContent = EGL_FALSE;
            surf->MipmapLevel = 0;
   surf->MultisampleResolve = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
            surf->HorizontalResolution = EGL_UNKNOWN;
   surf->VerticalResolution = EGL_UNKNOWN;
            surf->PostSubBufferSupportedNV = EGL_FALSE;
   surf->SetDamageRegionCalled = EGL_FALSE;
            /* the default swap interval is 1 */
            surf->HdrMetadata.display_primary_r.x = EGL_DONT_CARE;
   surf->HdrMetadata.display_primary_r.y = EGL_DONT_CARE;
   surf->HdrMetadata.display_primary_g.x = EGL_DONT_CARE;
   surf->HdrMetadata.display_primary_g.y = EGL_DONT_CARE;
   surf->HdrMetadata.display_primary_b.x = EGL_DONT_CARE;
   surf->HdrMetadata.display_primary_b.y = EGL_DONT_CARE;
   surf->HdrMetadata.white_point.x = EGL_DONT_CARE;
   surf->HdrMetadata.white_point.y = EGL_DONT_CARE;
   surf->HdrMetadata.max_luminance = EGL_DONT_CARE;
   surf->HdrMetadata.min_luminance = EGL_DONT_CARE;
   surf->HdrMetadata.max_cll = EGL_DONT_CARE;
            err = _eglParseSurfaceAttribList(surf, attrib_list);
   if (err != EGL_SUCCESS)
            /* if EGL_LARGEST_PBUFFER in use, clamp width and height */
   if (surf->LargestPbuffer) {
      surf->Width = MIN2(surf->Width, _EGL_MAX_PBUFFER_WIDTH);
                           }
      EGLBoolean
   _eglQuerySurface(_EGLDisplay *disp, _EGLSurface *surface, EGLint attribute,
         {
      switch (attribute) {
   case EGL_WIDTH:
      *value = surface->Width;
      case EGL_HEIGHT:
      *value = surface->Height;
      case EGL_CONFIG_ID:
      *value = surface->Config->ConfigID;
      case EGL_LARGEST_PBUFFER:
      if (surface->Type == EGL_PBUFFER_BIT)
            case EGL_TEXTURE_FORMAT:
      /* texture attributes: only for pbuffers, no error otherwise */
   if (surface->Type == EGL_PBUFFER_BIT)
            case EGL_TEXTURE_TARGET:
      if (surface->Type == EGL_PBUFFER_BIT)
            case EGL_MIPMAP_TEXTURE:
      if (surface->Type == EGL_PBUFFER_BIT)
            case EGL_MIPMAP_LEVEL:
      if (surface->Type == EGL_PBUFFER_BIT)
            case EGL_SWAP_BEHAVIOR:
      *value = surface->SwapBehavior;
      case EGL_RENDER_BUFFER:
      /* From the EGL_KHR_mutable_render_buffer spec (v12):
   *
   *    Querying EGL_RENDER_BUFFER returns the buffer which client API
   *    rendering is requested to use. For a window surface, this is the
   *    attribute value specified when the surface was created or last set
   *    via eglSurfaceAttrib.
   *
   * In other words, querying a window surface returns the value most
   * recently *requested* by the user.
   *
   * The paragraph continues in the EGL 1.5 spec (2014.08.27):
   *
   *    For a pbuffer surface, it is always EGL_BACK_BUFFER . For a pixmap
   *    surface, it is always EGL_SINGLE_BUFFER . To determine the actual
   *    buffer being rendered to by a context, call eglQueryContext.
   */
   switch (surface->Type) {
   default:
         case EGL_WINDOW_BIT:
      *value = surface->RequestedRenderBuffer;
      case EGL_PBUFFER_BIT:
      *value = EGL_BACK_BUFFER;
      case EGL_PIXMAP_BIT:
      *value = EGL_SINGLE_BUFFER;
      }
      case EGL_PIXEL_ASPECT_RATIO:
      *value = surface->AspectRatio;
      case EGL_HORIZONTAL_RESOLUTION:
      *value = surface->HorizontalResolution;
      case EGL_VERTICAL_RESOLUTION:
      *value = surface->VerticalResolution;
      case EGL_MULTISAMPLE_RESOLVE:
      *value = surface->MultisampleResolve;
      case EGL_VG_ALPHA_FORMAT:
      *value = surface->VGAlphaFormat;
      case EGL_VG_COLORSPACE:
      *value = surface->VGColorspace;
      case EGL_GL_COLORSPACE_KHR:
      if (!disp->Extensions.KHR_gl_colorspace)
            *value = surface->GLColorspace;
      case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
      *value = surface->PostSubBufferSupportedNV;
      case EGL_BUFFER_AGE_EXT:
      /* Both EXT_buffer_age and KHR_partial_update accept EGL_BUFFER_AGE_EXT.
   * To be precise, the KHR one accepts EGL_BUFFER_AGE_KHR which is an
   * alias with the same numeric value.
   */
   if (!disp->Extensions.EXT_buffer_age &&
                  _EGLContext *ctx = _eglGetCurrentContext();
   if (_eglGetContextHandle(ctx) == EGL_NO_CONTEXT ||
                  EGLint result = disp->Driver->QueryBufferAge(disp, surface);
   if (result < 0)
            if (disp->Options.GalliumHudWarn && result > 0) {
      _eglLog(_EGL_WARNING,
         "GALLIUM_HUD is not accounted for when querying "
   "buffer age, possibly causing artifacts, try running with "
   "MESA_EXTENSION_OVERRIDE=\"-EGL_EXT_buffer_age "
               *value = result;
   surface->BufferAgeRead = EGL_TRUE;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_RX_EXT:
      *value = surface->HdrMetadata.display_primary_r.x;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_RY_EXT:
      *value = surface->HdrMetadata.display_primary_r.y;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_GX_EXT:
      *value = surface->HdrMetadata.display_primary_g.x;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_GY_EXT:
      *value = surface->HdrMetadata.display_primary_g.y;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_BX_EXT:
      *value = surface->HdrMetadata.display_primary_b.x;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_BY_EXT:
      *value = surface->HdrMetadata.display_primary_b.y;
      case EGL_SMPTE2086_WHITE_POINT_X_EXT:
      *value = surface->HdrMetadata.white_point.x;
      case EGL_SMPTE2086_WHITE_POINT_Y_EXT:
      *value = surface->HdrMetadata.white_point.y;
      case EGL_SMPTE2086_MAX_LUMINANCE_EXT:
      *value = surface->HdrMetadata.max_luminance;
      case EGL_SMPTE2086_MIN_LUMINANCE_EXT:
      *value = surface->HdrMetadata.min_luminance;
      case EGL_CTA861_3_MAX_CONTENT_LIGHT_LEVEL_EXT:
      *value = surface->HdrMetadata.max_cll;
      case EGL_CTA861_3_MAX_FRAME_AVERAGE_LEVEL_EXT:
      *value = surface->HdrMetadata.max_fall;
      case EGL_PROTECTED_CONTENT_EXT:
      if (!disp->Extensions.EXT_protected_content &&
      !disp->Extensions.EXT_protected_surface)
      *value = surface->ProtectedContent;
      case EGL_PRESENT_OPAQUE_EXT:
      if (!disp->Extensions.EXT_present_opaque)
         *value = surface->PresentOpaque;
      default:
                     }
      /**
   * Default fallback routine - drivers might override this.
   */
   EGLBoolean
   _eglSurfaceAttrib(_EGLDisplay *disp, _EGLSurface *surface, EGLint attribute,
         {
      EGLint confval;
   EGLint err = EGL_SUCCESS;
   EGLint all_es_bits =
            switch (attribute) {
   case EGL_MIPMAP_LEVEL:
      confval = surface->Config->RenderableType;
   if (!(confval & all_es_bits)) {
      err = EGL_BAD_PARAMETER;
      }
   surface->MipmapLevel = value;
      case EGL_MULTISAMPLE_RESOLVE:
      switch (value) {
   case EGL_MULTISAMPLE_RESOLVE_DEFAULT:
         case EGL_MULTISAMPLE_RESOLVE_BOX:
      confval = surface->Config->SurfaceType;
   if (!(confval & EGL_MULTISAMPLE_RESOLVE_BOX_BIT))
            default:
      err = EGL_BAD_ATTRIBUTE;
      }
   if (err != EGL_SUCCESS)
         surface->MultisampleResolve = value;
      case EGL_RENDER_BUFFER:
      if (!disp->Extensions.KHR_mutable_render_buffer) {
      err = EGL_BAD_ATTRIBUTE;
               if (value != EGL_BACK_BUFFER && value != EGL_SINGLE_BUFFER) {
      err = EGL_BAD_PARAMETER;
               /* From the EGL_KHR_mutable_render_buffer spec (v12):
   *
   *    If attribute is EGL_RENDER_BUFFER, and the EGL_SURFACE_TYPE
   *    attribute of the EGLConfig used to create surface does not contain
   *    EGL_MUTABLE_RENDER_BUFFER_BIT_KHR, [...] an EGL_BAD_MATCH error is
   *    generated [...].
   */
   if (!(surface->Config->SurfaceType & EGL_MUTABLE_RENDER_BUFFER_BIT_KHR)) {
      err = EGL_BAD_MATCH;
               surface->RequestedRenderBuffer = value;
      case EGL_SWAP_BEHAVIOR:
      switch (value) {
   case EGL_BUFFER_DESTROYED:
         case EGL_BUFFER_PRESERVED:
      confval = surface->Config->SurfaceType;
   if (!(confval & EGL_SWAP_BEHAVIOR_PRESERVED_BIT))
            default:
      err = EGL_BAD_ATTRIBUTE;
      }
   if (err != EGL_SUCCESS)
         surface->SwapBehavior = value;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_RX_EXT:
      surface->HdrMetadata.display_primary_r.x = value;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_RY_EXT:
      surface->HdrMetadata.display_primary_r.y = value;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_GX_EXT:
      surface->HdrMetadata.display_primary_g.x = value;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_GY_EXT:
      surface->HdrMetadata.display_primary_g.y = value;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_BX_EXT:
      surface->HdrMetadata.display_primary_b.x = value;
      case EGL_SMPTE2086_DISPLAY_PRIMARY_BY_EXT:
      surface->HdrMetadata.display_primary_b.y = value;
      case EGL_SMPTE2086_WHITE_POINT_X_EXT:
      surface->HdrMetadata.white_point.x = value;
      case EGL_SMPTE2086_WHITE_POINT_Y_EXT:
      surface->HdrMetadata.white_point.y = value;
      case EGL_SMPTE2086_MAX_LUMINANCE_EXT:
      surface->HdrMetadata.max_luminance = value;
      case EGL_SMPTE2086_MIN_LUMINANCE_EXT:
      surface->HdrMetadata.min_luminance = value;
      case EGL_CTA861_3_MAX_CONTENT_LIGHT_LEVEL_EXT:
      surface->HdrMetadata.max_cll = value;
      case EGL_CTA861_3_MAX_FRAME_AVERAGE_LEVEL_EXT:
      surface->HdrMetadata.max_fall = value;
      default:
      err = EGL_BAD_ATTRIBUTE;
               if (err != EGL_SUCCESS)
            }
      EGLBoolean
   _eglBindTexImage(_EGLDisplay *disp, _EGLSurface *surface, EGLint buffer)
   {
               /* Just do basic error checking and return success/fail.
   * Drivers must implement the real stuff.
            if (disp->Extensions.NOK_texture_from_pixmap)
            if (!(surface->Type & texture_type))
            if (surface->TextureFormat == EGL_NO_TEXTURE)
            if (surface->TextureTarget == EGL_NO_TEXTURE)
            if (buffer != EGL_BACK_BUFFER)
                        }
      EGLBoolean
   _eglReleaseTexImage(_EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
   {
      /* Just do basic error checking and return success/fail.
   * Drivers must implement the real stuff.
                     if (surf == EGL_NO_SURFACE)
            if (!surf->BoundToTexture) {
      /* Not an error, simply nothing to do */
               if (surf->TextureFormat == EGL_NO_TEXTURE)
            if (buffer != EGL_BACK_BUFFER)
            if (disp->Extensions.NOK_texture_from_pixmap)
            if (!(surf->Type & texture_type))
                        }
      EGLBoolean
   _eglSurfaceHasMutableRenderBuffer(_EGLSurface *surf)
   {
      return surf->Type == EGL_WINDOW_BIT && surf->Config &&
      }
      EGLBoolean
   _eglSurfaceInSharedBufferMode(_EGLSurface *surf)
   {
      return _eglSurfaceHasMutableRenderBuffer(surf) &&
      }
