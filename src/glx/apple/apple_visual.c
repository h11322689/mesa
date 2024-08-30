   /*
   Copyright (c) 2008, 2009 Apple Inc.
      Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation files
   (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:
      The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
   HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
      Except as contained in this notice, the name(s) of the above
   copyright holders shall not be used in advertising or otherwise to
   promote the sale, use or other dealings in this Software without
   prior written authorization.
   */
      #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <assert.h>
   #include <GL/gl.h>
   #include <util/u_debug.h>
      /* <rdar://problem/6953344> */
   #define glTexImage1D glTexImage1D_OSX
   #define glTexImage2D glTexImage2D_OSX
   #define glTexImage3D glTexImage3D_OSX
   #include <OpenGL/OpenGL.h>
   #include <OpenGL/CGLContext.h>
   #include <OpenGL/CGLRenderers.h>
   #include <OpenGL/CGLTypes.h>
   #undef glTexImage1D
   #undef glTexImage2D
   #undef glTexImage3D
      #ifndef kCGLPFAOpenGLProfile
   #define kCGLPFAOpenGLProfile 99
   #endif
      #ifndef kCGLOGLPVersion_3_2_Core
   #define kCGLOGLPVersion_3_2_Core 0x3200
   #endif
      #include "apple_cgl.h"
   #include "apple_visual.h"
   #include "apple_glx.h"
   #include "glxconfig.h"
      enum
   {
         };
      static char __crashreporter_info_buff__[4096] = { 0 };
   static const char *__crashreporter_info__ __attribute__((__used__)) =
         #if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
   // This is actually a toolchain requirement, but I'm not sure the correct check,
   // but it should be fine to just only include it for Leopard and later.  This line
   // just tells the linker to never strip this symbol (such as for space optimization)
   __asm__ (".desc ___crashreporter_info__, 0x10");
   #endif
      void
   apple_visual_create_pfobj(CGLPixelFormatObj * pfobj, const struct glx_config * mode,
               {
      CGLPixelFormatAttribute attr[MAX_ATTR];
   int numattr = 0;
   GLint vsref = 0;
   CGLError error = 0;
            if (offscreen) {
      apple_glx_diagnostic
               }
   else if (debug_get_bool_option("LIBGL_ALWAYS_SOFTWARE", false)) {
      apple_glx_diagnostic
         attr[numattr++] = kCGLPFARendererID;
      }
   else if (debug_get_bool_option("LIBGL_ALLOW_SOFTWARE", false)) {
      apple_glx_diagnostic
      }
   else {
                  /* 
   * The program chose a config based on the fbconfigs or visuals.
   * Those are based on the attributes from CGL, so we probably
   * do want the closest match for the color, depth, and accum.
   */
            if (mode->stereoMode) {
      attr[numattr++] = kCGLPFAStereo;
      }
   else {
                  if (!offscreen && mode->doubleBufferMode) {
      attr[numattr++] = kCGLPFADoubleBuffer;
      }
   else {
                  attr[numattr++] = kCGLPFAColorSize;
   attr[numattr++] = mode->redBits + mode->greenBits + mode->blueBits;
   attr[numattr++] = kCGLPFAAlphaSize;
            if ((mode->accumRedBits + mode->accumGreenBits + mode->accumBlueBits) > 0) {
      attr[numattr++] = kCGLPFAAccumSize;
   attr[numattr++] = mode->accumRedBits + mode->accumGreenBits +
               if (mode->depthBits > 0) {
      attr[numattr++] = kCGLPFADepthSize;
               if (mode->stencilBits > 0) {
      attr[numattr++] = kCGLPFAStencilSize;
               if (mode->sampleBuffers > 0) {
      attr[numattr++] = kCGLPFAMultisample;
   attr[numattr++] = kCGLPFASampleBuffers;
   attr[numattr++] = mode->sampleBuffers;
   attr[numattr++] = kCGLPFASamples;
               /* Debugging support for Core profiles to support newer versions of OpenGL */
   if (use_core_profile) {
      attr[numattr++] = kCGLPFAOpenGLProfile;
                                          if ((error == kCGLBadAttribute || vsref == 0) && use_core_profile) {
      apple_glx_diagnostic
            if (!error)
            numattr -= 3;
                        if (error) {
      snprintf(__crashreporter_info_buff__, sizeof(__crashreporter_info_buff__),
         fprintf(stderr, "%s", __crashreporter_info_buff__);
               if (!*pfobj) {
      snprintf(__crashreporter_info_buff__, sizeof(__crashreporter_info_buff__),
         fprintf(stderr, "%s", __crashreporter_info_buff__);
         }
