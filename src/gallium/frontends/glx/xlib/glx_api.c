   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
   * "Fake" GLX API implemented in terms of the XMesa*() functions.
   */
            #define GLX_GLXEXT_PROTOTYPES
   #include "GL/glx.h"
      #include <stdio.h>
   #include <string.h>
   #include <X11/Xmd.h>
   #include <GL/glxproto.h>
      #include "xm_api.h"
   #include "main/errors.h"
   #include "main/config.h"
   #include "util/compiler.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      /* An "Atrribs/Attribs" typo was fixed in glxproto.h in Nov 2014.
   * This is in case we don't have the updated header.
   */
   #if !defined(X_GLXCreateContextAttribsARB) && \
         #define X_GLXCreateContextAttribsARB X_GLXCreateContextAtrribsARB
   #endif
      /* This indicates the client-side GLX API and GLX encoder version. */
   #define CLIENT_MAJOR_VERSION 1
   #define CLIENT_MINOR_VERSION 4  /* but don't have 1.3's pbuffers, etc yet */
      /* This indicates the server-side GLX decoder version.
   * GLX 1.4 indicates OpenGL 1.3 support
   */
   #define SERVER_MAJOR_VERSION 1
   #define SERVER_MINOR_VERSION 4
      /* Who implemented this GLX? */
   #define VENDOR "Brian Paul"
      #define EXTENSIONS \
      "GLX_MESA_copy_sub_buffer " \
   "GLX_MESA_pixmap_colormap " \
   "GLX_MESA_release_buffers " \
   "GLX_ARB_create_context " \
   "GLX_ARB_create_context_profile " \
   "GLX_ARB_get_proc_address " \
   "GLX_EXT_create_context_es_profile " \
   "GLX_EXT_create_context_es2_profile " \
   "GLX_EXT_texture_from_pixmap " \
   "GLX_EXT_visual_info " \
   "GLX_EXT_visual_rating " \
   /*"GLX_SGI_video_sync "*/ \
   "GLX_SGIX_fbconfig " \
         #define DEFAULT_DIRECT GL_TRUE
         /** XXX this could be based on gallium's max texture size */
   #define PBUFFER_MAX_SIZE 16384
         /**
   * The GLXContext typedef is defined as a pointer to this structure.
   */
   struct __GLXcontextRec
   {
      Display *currentDpy;
   GLboolean isDirect;
   GLXDrawable currentDrawable;
   GLXDrawable currentReadable;
               };
         thread_local GLXContext ContextTSD;
      /** Set current context for calling thread */
   static void
   SetCurrentContext(GLXContext c)
   {
         }
      /** Get current context for calling thread */
   static GLXContext
   GetCurrentContext(void)
   {
         }
            /**********************************************************************/
   /***                       GLX Visual Code                          ***/
   /**********************************************************************/
      #define DONT_CARE -1
         static XMesaVisual *VisualTable = NULL;
   static int NumVisuals = 0;
            /* Macro to handle c_class vs class field name in XVisualInfo struct */
   #if defined(__cplusplus) || defined(c_plusplus)
   #define CLASS c_class
   #else
   #define CLASS class
   #endif
            /*
   * Test if the given XVisualInfo is usable for Mesa rendering.
   */
   static GLboolean
   is_usable_visual( XVisualInfo *vinfo )
   {
      switch (vinfo->CLASS) {
      case StaticGray:
   case GrayScale:
      /* Any StaticGray/GrayScale visual works in RGB or CI mode */
      case StaticColor:
   /* Any StaticColor/PseudoColor visual of at least 4 bits */
   if (vinfo->depth>=4) {
         }
   else {
         }
         case TrueColor:
   /* Any depth of TrueColor or DirectColor works in RGB mode */
   return GL_TRUE;
         /* This should never happen */
   return GL_FALSE;
         }
         /*
   * Given an XVisualInfo and RGB, Double, and Depth buffer flags, save the
   * configuration in our list of GLX visuals.
   */
   static XMesaVisual
   save_glx_visual( Display *dpy, XVisualInfo *vinfo,
                  GLboolean rgbFlag, GLboolean alphaFlag, GLboolean dbFlag,
   GLboolean stereoFlag,
   GLint depth_size, GLint stencil_size,
      {
      GLboolean ximageFlag = GL_TRUE;
   XMesaVisual xmvis;
   GLint i;
            if (!rgbFlag)
            if (dbFlag) {
      /* Check if the MESA_BACK_BUFFER env var is set */
   char *backbuffer = getenv("MESA_BACK_BUFFER");
   if (backbuffer) {
      if (backbuffer[0]=='p' || backbuffer[0]=='P') {
         }
   else if (backbuffer[0]=='x' || backbuffer[0]=='X') {
         }
   else {
                        if (stereoFlag) {
      /* stereo not supported */
               if (stencil_size > 0 && depth_size > 0)
            /* Comparing IDs uses less memory but sometimes fails. */
   /* XXX revisit this after 3.0 is finished. */
   if (getenv("MESA_GLX_VISUAL_HACK"))
         else
            /* Force the visual to have an alpha channel */
   if (rgbFlag && getenv("MESA_GLX_FORCE_ALPHA"))
            /* First check if a matching visual is already in the list */
   for (i=0; i<NumVisuals; i++) {
      XMesaVisual v = VisualTable[i];
   if (v->display == dpy
      && v->mesa_visual.samples == num_samples
   && v->ximage_flag == ximageFlag
   && v->mesa_visual.doubleBufferMode == dbFlag
   && v->mesa_visual.stereoMode == stereoFlag
   && (v->mesa_visual.alphaBits > 0) == alphaFlag
   && (v->mesa_visual.depthBits >= depth_size || depth_size == 0)
   && (v->mesa_visual.stencilBits >= stencil_size || stencil_size == 0)
   && (v->mesa_visual.accumRedBits >= accumRedSize || accumRedSize == 0)
   && (v->mesa_visual.accumGreenBits >= accumGreenSize || accumGreenSize == 0)
   && (v->mesa_visual.accumBlueBits >= accumBlueSize || accumBlueSize == 0)
   && (v->mesa_visual.accumAlphaBits >= accumAlphaSize || accumAlphaSize == 0)) {
   /* now either compare XVisualInfo pointers or visual IDs */
   if ((!comparePointers && v->visinfo->visualid == vinfo->visualid)
      || (comparePointers && v->vishandle == vinfo)) {
                              xmvis = XMesaCreateVisual( dpy, vinfo, rgbFlag, alphaFlag, dbFlag,
                                 if (xmvis) {
      /* Save a copy of the pointer now so we can find this visual again
   * if we need to search for it in find_glx_visual().
   */
   xmvis->vishandle = vinfo;
   /* Allocate more space for additional visual */
   VisualTable = realloc(VisualTable, sizeof(XMesaVisual) * (NumVisuals + 1));
   /* add xmvis to the list */
   VisualTable[NumVisuals] = xmvis;
      }
      }
         /**
   * Return the default number of bits for the Z buffer.
   * If defined, use the MESA_GLX_DEPTH_BITS env var value.
   * Otherwise, use 24.
   * XXX probably do the same thing for stencil, accum, etc.
   */
   static GLint
   default_depth_bits(void)
   {
      int zBits;
   const char *zEnv = getenv("MESA_GLX_DEPTH_BITS");
   if (zEnv)
         else
            }
      static GLint
   default_alpha_bits(void)
   {
      int aBits;
   const char *aEnv = getenv("MESA_GLX_ALPHA_BITS");
   if (aEnv)
         else
            }
      static GLint
   default_accum_bits(void)
   {
         }
            /*
   * Create a GLX visual from a regular XVisualInfo.
   * This is called when Fake GLX is given an XVisualInfo which wasn't
   * returned by glXChooseVisual.  Since this is the first time we're
   * considering this visual we'll take a guess at reasonable values
   * for depth buffer size, stencil size, accum size, etc.
   * This is the best we can do with a client-side emulation of GLX.
   */
   static XMesaVisual
   create_glx_visual( Display *dpy, XVisualInfo *visinfo )
   {
      GLint zBits = default_depth_bits();
   GLint accBits = default_accum_bits();
            if (is_usable_visual( visinfo )) {
      /* Configure this visual as RGB, double-buffered, depth-buffered. */
   /* This is surely wrong for some people's needs but what else */
   /* can be done?  They should use glXChooseVisual(). */
   return save_glx_visual( dpy, visinfo,
                           GL_TRUE,   /* rgb */
   alphaFlag, /* alpha */
   GL_TRUE,   /* double */
   GL_FALSE,  /* stereo */
   zBits,
   8,       /* stencil bits */
   accBits, /* r */
   accBits, /* g */
   accBits, /* b */
      }
   else {
      _mesa_warning(NULL, "Mesa: error in glXCreateContext: bad visual\n");
         }
            /*
   * Find the GLX visual associated with an XVisualInfo.
   */
   static XMesaVisual
   find_glx_visual( Display *dpy, XVisualInfo *vinfo )
   {
               /* try to match visual id */
   for (i=0;i<NumVisuals;i++) {
      if (VisualTable[i]->display==dpy
      && VisualTable[i]->visinfo->visualid == vinfo->visualid) {
                  /* if that fails, try to match pointers */
   for (i=0;i<NumVisuals;i++) {
      if (VisualTable[i]->display==dpy && VisualTable[i]->vishandle==vinfo) {
                        }
         /**
   * Try to get an X visual which matches the given arguments.
   */
   static XVisualInfo *
   get_visual( Display *dpy, int scr, unsigned int depth, int xclass )
   {
      XVisualInfo temp, *vis;
   long mask;
   int n;
   unsigned int default_depth;
            mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
   temp.screen = scr;
   temp.depth = depth;
            default_depth = DefaultDepth(dpy,scr);
            if (depth==default_depth && xclass==default_class) {
      /* try to get root window's visual */
   temp.visualid = DefaultVisual(dpy,scr)->visualid;
                        /* In case bits/pixel > 24, make sure color channels are still <=8 bits.
   * An SGI Infinite Reality system, for example, can have 30bpp pixels:
   * 10 bits per color channel.  Mesa's limited to a max of 8 bits/channel.
   */
   if (vis && depth > 24 && (xclass==TrueColor || xclass==DirectColor)) {
      if (util_bitcount((GLuint) vis->red_mask  ) <= 8 &&
      util_bitcount((GLuint) vis->green_mask) <= 8 &&
   util_bitcount((GLuint) vis->blue_mask ) <= 8) {
      }
   else {
      XFree((void *) vis);
                     }
         /*
   * Retrieve the value of the given environment variable and find
   * the X visual which matches it.
   * Input:  dpy - the display
   *         screen - the screen number
   *         varname - the name of the environment variable
   * Return:  an XVisualInfo pointer to NULL if error.
   */
   static XVisualInfo *
   get_env_visual(Display *dpy, int scr, const char *varname)
   {
      char value[100], type[100];
   int depth, xclass = -1;
            if (!getenv( varname )) {
                  strncpy( value, getenv(varname), 100 );
                     if (strcmp(type,"TrueColor")==0)          xclass = TrueColor;
   else if (strcmp(type,"DirectColor")==0)   xclass = DirectColor;
   else if (strcmp(type,"PseudoColor")==0)   xclass = PseudoColor;
   else if (strcmp(type,"StaticColor")==0)   xclass = StaticColor;
   else if (strcmp(type,"GrayScale")==0)     xclass = GrayScale;
            if (xclass>-1 && depth>0) {
      vis = get_visual( dpy, scr, depth, xclass );
   return vis;
                     _mesa_warning(NULL, "GLX unable to find visual class=%s, depth=%d.",
               }
            /*
   * Select an X visual which satisfies the RGBA flag and minimum depth.
   * Input:  dpy,
   *         screen - X display and screen number
   *         min_depth - minimum visual depth
   *         preferred_class - preferred GLX visual class or DONT_CARE
   * Return:  pointer to an XVisualInfo or NULL.
   */
   static XVisualInfo *
   choose_x_visual( Display *dpy, int screen, int min_depth,
         {
      XVisualInfo *vis;
   int xclass, visclass = 0;
            /* First see if the MESA_RGB_VISUAL env var is defined */
   vis = get_env_visual( dpy, screen, "MESA_RGB_VISUAL" );
   if (vis) {
         }
   /* Otherwise, search for a suitable visual */
   if (preferred_class==DONT_CARE) {
      for (xclass=0;xclass<6;xclass++) {
      switch (xclass) {
   case 0:  visclass = TrueColor;    break;
   case 1:  visclass = DirectColor;  break;
   case 2:  visclass = PseudoColor;  break;
   case 3:  visclass = StaticColor;  break;
   case 4:  visclass = GrayScale;    break;
   case 5:  visclass = StaticGray;   break;
   }
   if (min_depth==0) {
      /* start with shallowest */
   for (depth=0;depth<=32;depth++) {
      if (visclass==TrueColor && depth==8) {
      /* Special case:  try to get 8-bit PseudoColor before */
   /* 8-bit TrueColor */
   vis = get_visual( dpy, screen, 8, PseudoColor );
   if (vis) {
            }
   vis = get_visual( dpy, screen, depth, visclass );
   if (vis) {
               }
   else {
      /* start with deepest */
   for (depth=32;depth>=min_depth;depth--) {
      if (visclass==TrueColor && depth==8) {
      /* Special case:  try to get 8-bit PseudoColor before */
   /* 8-bit TrueColor */
   vis = get_visual( dpy, screen, 8, PseudoColor );
   if (vis) {
            }
   vis = get_visual( dpy, screen, depth, visclass );
   if (vis) {
                     }
   else {
      /* search for a specific visual class */
   switch (preferred_class) {
   case GLX_TRUE_COLOR_EXT:    visclass = TrueColor;    break;
   case GLX_DIRECT_COLOR_EXT:  visclass = DirectColor;  break;
   case GLX_PSEUDO_COLOR_EXT:  visclass = PseudoColor;  break;
   case GLX_STATIC_COLOR_EXT:  visclass = StaticColor;  break;
   case GLX_GRAY_SCALE_EXT:    visclass = GrayScale;    break;
   case GLX_STATIC_GRAY_EXT:   visclass = StaticGray;   break;
   default:   return NULL;
   }
   if (min_depth==0) {
      /* start with shallowest */
   for (depth=0;depth<=32;depth++) {
      vis = get_visual( dpy, screen, depth, visclass );
   if (vis) {
               }
   else {
      /* start with deepest */
   for (depth=32;depth>=min_depth;depth--) {
      vis = get_visual( dpy, screen, depth, visclass );
   if (vis) {
                           /* didn't find a visual */
      }
               /**********************************************************************/
   /***             Display-related functions                          ***/
   /**********************************************************************/
         /**
   * Free all XMesaVisuals which are associated with the given display.
   */
   static void
   destroy_visuals_on_display(Display *dpy)
   {
      int i;
   for (i = 0; i < NumVisuals; i++) {
      if (VisualTable[i]->display == dpy) {
      /* remove this visual */
   int j;
   free(VisualTable[i]);
   for (j = i; j < NumVisuals - 1; j++)
                  }
         /**
   * Called from XCloseDisplay() to let us free our display-related data.
   */
   static int
   close_display_callback(Display *dpy, XExtCodes *codes)
   {
      xmesa_destroy_buffers_on_display(dpy);
   destroy_visuals_on_display(dpy);
   xmesa_close_display(dpy);
      }
         /**
   * Look for the named extension on given display and return a pointer
   * to the _XExtension data, or NULL if extension not found.
   */
   static _XExtension *
   lookup_extension(Display *dpy, const char *extName)
   {
      _XExtension *ext;
   for (ext = dpy->ext_procs; ext; ext = ext->next) {
      if (ext->name && strcmp(ext->name, extName) == 0) {
            }
      }
         /**
   * Whenever we're given a new Display pointer, call this function to
   * register our close_display_callback function.
   */
   static void
   register_with_display(Display *dpy)
   {
      const char *extName = "MesaGLX";
            ext = lookup_extension(dpy, extName);
   if (!ext) {
      XExtCodes *c = XAddExtension(dpy);
   ext = dpy->ext_procs;  /* new extension is at head of list */
   assert(c->extension == ext->codes.extension);
   (void) c;
   ext->name = strdup(extName);
         }
         /**
   * Fake an error.
   */
   static int
   generate_error(Display *dpy,
                  unsigned char error_code,
      {
      XErrorHandler handler;
   int major_opcode;
   int first_event;
   int first_error;
            handler = XSetErrorHandler(NULL);
   XSetErrorHandler(handler);
   if (!handler) {
                  if (!XQueryExtension(dpy, GLX_EXTENSION_NAME, &major_opcode, &first_event, &first_error)) {
      major_opcode = 0;
   first_event = 0;
               if (!core) {
                           event.xerror.type = X_Error;
   event.xerror.display = dpy;
   event.xerror.resourceid = resourceid;
   event.xerror.serial = NextRequest(dpy) - 1;
   event.xerror.error_code = error_code;
   event.xerror.request_code = major_opcode;
               }
         /**********************************************************************/
   /***                  Begin Fake GLX API Functions                  ***/
   /**********************************************************************/
         /**
   * Helper used by glXChooseVisual and glXChooseFBConfig.
   * The fbConfig parameter must be GL_FALSE for the former and GL_TRUE for
   * the later.
   * In either case, the attribute list is terminated with the value 'None'.
   */
   static XMesaVisual
   choose_visual( Display *dpy, int screen, const int *list, GLboolean fbConfig )
   {
      const GLboolean rgbModeDefault = fbConfig;
   const int *parselist;
   XVisualInfo *vis;
   int min_red=0, min_green=0, min_blue=0;
   GLboolean rgb_flag = rgbModeDefault;
   GLboolean alpha_flag = GL_FALSE;
   GLboolean double_flag = GL_FALSE;
   GLboolean stereo_flag = GL_FALSE;
   GLint depth_size = 0;
   GLint stencil_size = 0;
   GLint accumRedSize = 0;
   GLint accumGreenSize = 0;
   GLint accumBlueSize = 0;
   GLint accumAlphaSize = 0;
   int level = 0;
   int visual_type = DONT_CARE;
   GLint caveat = DONT_CARE;
   XMesaVisual xmvis = NULL;
   int desiredVisualID = -1;
   int numAux = 0;
            if (xmesa_init( dpy ) != 0) {
      _mesa_warning(NULL, "Failed to initialize display");
                                    if (fbConfig &&
      parselist[1] == GLX_DONT_CARE &&
   parselist[0] != GLX_LEVEL) {
   /* For glXChooseFBConfig(), skip attributes whose value is
   * GLX_DONT_CARE, unless it's GLX_LEVEL (which can legitimately be
   * a negative value).
   *
   * From page 17 (23 of the pdf) of the GLX 1.4 spec:
   * GLX DONT CARE may be specified for all attributes except GLX LEVEL.
   */
   parselist += 2;
               case GLX_USE_GL:
               if (fbConfig) {
      /* invalid token */
      }
   else {
      /* skip */
      case GLX_BUFFER_SIZE:
      parselist++;
   parselist++;
      case GLX_LEVEL:
      parselist++;
            case GLX_RGBA:
               if (fbConfig) {
      /* invalid token */
      }
   else {
      rgb_flag = GL_TRUE;
      case GLX_DOUBLEBUFFER:
               parselist++;
   if (fbConfig) {
         }
   else {
         case GLX_STEREO:
               parselist++;
   if (fbConfig) {
         }
   else {
         case GLX_AUX_BUFFERS:
      parselist++;
            numAux = *parselist++;
         case GLX_RED_SIZE:
      parselist++;
   min_red = *parselist++;
      case GLX_GREEN_SIZE:
      parselist++;
   min_green = *parselist++;
      case GLX_BLUE_SIZE:
      parselist++;
   min_blue = *parselist++;
      case GLX_ALPHA_SIZE:
      parselist++;
            {
      GLint size = *parselist++;
      case GLX_DEPTH_SIZE:
      parselist++;
   depth_size = *parselist++;
      case GLX_STENCIL_SIZE:
      parselist++;
   stencil_size = *parselist++;
      case GLX_ACCUM_RED_SIZE:
      parselist++;
            {
      GLint size = *parselist++;
      case GLX_ACCUM_GREEN_SIZE:
      parselist++;
            {
      GLint size = *parselist++;
      case GLX_ACCUM_BLUE_SIZE:
      parselist++;
            {
      GLint size = *parselist++;
      case GLX_ACCUM_ALPHA_SIZE:
      parselist++;
            {
      GLint size = *parselist++;
                  /*
   * GLX_EXT_visual_info extension
   */
   case GLX_X_VISUAL_TYPE_EXT:
      parselist++;
   visual_type = *parselist++;
      case GLX_TRANSPARENT_TYPE_EXT:
      parselist++;
   parselist++;
      case GLX_TRANSPARENT_INDEX_VALUE_EXT:
      parselist++;
   parselist++;
      case GLX_TRANSPARENT_RED_VALUE_EXT:
   case GLX_TRANSPARENT_GREEN_VALUE_EXT:
   case GLX_TRANSPARENT_BLUE_VALUE_EXT:
   /* ignore */
   parselist++;
   parselist++;
                  /*
   * GLX_EXT_visual_info extension
   */
   case GLX_VISUAL_CAVEAT_EXT:
      parselist++;
               /*
   * GLX_ARB_multisample
   */
   case GLX_SAMPLE_BUFFERS_ARB:
      /* ignore */
   parselist++;
   parselist++;
      case GLX_SAMPLES_ARB:
      parselist++;
               /*
   * FBConfig attribs.
   */
   case GLX_RENDER_TYPE:
      if (!fbConfig)
         parselist++;
   if (*parselist & GLX_RGBA_BIT) {
         }
   else if (*parselist & GLX_COLOR_INDEX_BIT) {
         }
   else if (*parselist == 0) {
         }
   parselist++;
      case GLX_DRAWABLE_TYPE:
      if (!fbConfig)
         parselist++;
   if (*parselist & ~(GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT)) {
         }
   parselist++;
      case GLX_FBCONFIG_ID:
   case GLX_VISUAL_ID:
      if (!fbConfig)
         parselist++;
   desiredVisualID = *parselist++;
      case GLX_X_RENDERABLE:
   case GLX_MAX_PBUFFER_WIDTH:
   case GLX_MAX_PBUFFER_HEIGHT:
   case GLX_MAX_PBUFFER_PIXELS:
      if (!fbConfig)
                     case GLX_BIND_TO_TEXTURE_RGB_EXT:
      parselist++; /*skip*/
      case GLX_BIND_TO_TEXTURE_RGBA_EXT:
      parselist++; /*skip*/
      case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
      parselist++; /*skip*/
      case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
      parselist++;
   if (*parselist & ~(GLX_TEXTURE_1D_BIT_EXT |
                  /* invalid bit */
      }
      case GLX_Y_INVERTED_EXT:
         case None:
                  default:
      /* undefined attribute */
               return NULL;
                           if (num_samples < 0) {
      _mesa_warning(NULL, "GLX_SAMPLES_ARB: number of samples must not be negative");
               /*
   * Since we're only simulating the GLX extension this function will never
   * find any real GL visuals.  Instead, all we can do is try to find an RGB
   * or CI visual of appropriate depth.  Other requested attributes such as
   * double buffering, depth buffer, etc. will be associated with the X
   * visual and stored in the VisualTable[].
   */
   if (desiredVisualID != -1) {
      /* try to get a specific visual, by visualID */
   XVisualInfo temp;
   int n;
   temp.visualid = desiredVisualID;
   temp.screen = screen;
   vis = XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask, &temp, &n);
   if (vis) {
      /* give the visual some useful GLX attributes */
   double_flag = GL_TRUE;
         }
   else if (level==0) {
      /* normal color planes */
   /* Get an RGB visual */
   int min_rgb = min_red + min_green + min_blue;
   if (min_rgb>1 && min_rgb<8) {
      /* a special case to be sure we can get a monochrome visual */
      }
      }
   else {
      _mesa_warning(NULL, "overlay not supported");
               if (vis) {
      /* Note: we're not exactly obeying the glXChooseVisual rules here.
   * When GLX_DEPTH_SIZE = 1 is specified we're supposed to choose the
   * largest depth buffer size, which is 32bits/value.  Instead, we
   * return 16 to maintain performance with earlier versions of Mesa.
   */
   if (stencil_size > 0)
         else if (depth_size > 24)
         else if (depth_size > 16)
         else if (depth_size > 0) {
                  if (!alpha_flag) {
                  /* we only support one size of stencil and accum buffers. */
   if (stencil_size > 0)
            if (accumRedSize > 0 ||
      accumGreenSize > 0 ||
                  accumRedSize =
                              xmvis = save_glx_visual( dpy, vis, rgb_flag, alpha_flag, double_flag,
                                    }
         PUBLIC XVisualInfo *
   glXChooseVisual( Display *dpy, int screen, int *list )
   {
               /* register ourselves as an extension on this display */
            xmvis = choose_visual(dpy, screen, list, GL_FALSE);
   if (xmvis) {
      /* create a new vishandle - the cached one may be stale */
   xmvis->vishandle = malloc(sizeof(XVisualInfo));
   if (xmvis->vishandle) {
         }
      }
   else
      }
         /**
   * Helper function used by other glXCreateContext functions.
   */
   static GLXContext
   create_context(Display *dpy, XMesaVisual xmvis,
                     {
               if (!dpy || !xmvis)
            glxCtx = CALLOC_STRUCT(__GLXcontextRec);
   if (!glxCtx)
               #if 0
         #endif
         glxCtx->xmesaContext = XMesaCreateContext(xmvis, shareCtx, major, minor,
         if (!glxCtx->xmesaContext) {
      free(glxCtx);
               glxCtx->isDirect = DEFAULT_DIRECT;
   glxCtx->currentDpy = dpy;
               }
         PUBLIC GLXContext
   glXCreateContext( Display *dpy, XVisualInfo *visinfo,
         {
               xmvis = find_glx_visual( dpy, visinfo );
   if (!xmvis) {
      /* This visual wasn't found with glXChooseVisual() */
   xmvis = create_glx_visual( dpy, visinfo );
   if (!xmvis) {
      /* unusable visual */
                  return create_context(dpy, xmvis,
                  }
         /* GLX 1.3 and later */
   PUBLIC Bool
   glXMakeContextCurrent( Display *dpy, GLXDrawable draw,
         {
      GLXContext glxCtx = ctx;
   GLXContext current = GetCurrentContext();
            if (firsttime) {
      no_rast = getenv("SP_NO_RAST") != NULL;
               if (ctx) {
      XMesaBuffer drawBuffer = NULL, readBuffer = NULL;
            /* either both must be null, or both must be non-null */
   if (!draw != !read)
            if (draw) {
      /* Find the XMesaBuffer which corresponds to 'draw' */
   drawBuffer = XMesaFindBuffer( dpy, draw );
   if (!drawBuffer) {
      /* drawable must be a new window! */
   drawBuffer = XMesaCreateWindowBuffer( xmctx->xm_visual, draw );
   if (!drawBuffer) {
      /* Out of memory, or context/drawable depth mismatch */
                     if (read) {
      /* Find the XMesaBuffer which corresponds to 'read' */
   readBuffer = XMesaFindBuffer( dpy, read );
   if (!readBuffer) {
      /* drawable must be a new window! */
   readBuffer = XMesaCreateWindowBuffer( xmctx->xm_visual, read );
   if (!readBuffer) {
      /* Out of memory, or context/drawable depth mismatch */
                     if (no_rast && current == ctx)
            /* Now make current! */
   if (XMesaMakeCurrent2(xmctx, drawBuffer, readBuffer)) {
      ctx->currentDpy = dpy;
   ctx->currentDrawable = draw;
   ctx->currentReadable = read;
   SetCurrentContext(ctx);
      }
   else {
            }
   else if (!ctx && !draw && !read) {
      /* release current context w/out assigning new one. */
   XMesaMakeCurrent2( NULL, NULL, NULL );
   SetCurrentContext(NULL);
      }
   else {
      /* We were given an invalid set of arguments */
         }
         PUBLIC Bool
   glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx )
   {
         }
         PUBLIC GLXContext
   glXGetCurrentContext(void)
   {
         }
         PUBLIC Display *
   glXGetCurrentDisplay(void)
   {
                  }
         PUBLIC Display *
   glXGetCurrentDisplayEXT(void)
   {
         }
         PUBLIC GLXDrawable
   glXGetCurrentDrawable(void)
   {
      GLXContext gc = glXGetCurrentContext();
      }
         PUBLIC GLXDrawable
   glXGetCurrentReadDrawable(void)
   {
      GLXContext gc = glXGetCurrentContext();
      }
         PUBLIC GLXDrawable
   glXGetCurrentReadDrawableSGI(void)
   {
         }
         PUBLIC GLXPixmap
   glXCreateGLXPixmap( Display *dpy, XVisualInfo *visinfo, Pixmap pixmap )
   {
      XMesaVisual v;
            v = find_glx_visual( dpy, visinfo );
   if (!v) {
      v = create_glx_visual( dpy, visinfo );
   if (!v) {
      /* unusable visual */
                  b = XMesaCreatePixmapBuffer( v, pixmap, 0 );
   if (!b) {
         }
      }
         /*** GLX_MESA_pixmap_colormap ***/
      PUBLIC GLXPixmap
   glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visinfo,
         {
      XMesaVisual v;
            v = find_glx_visual( dpy, visinfo );
   if (!v) {
      v = create_glx_visual( dpy, visinfo );
   if (!v) {
      /* unusable visual */
                  b = XMesaCreatePixmapBuffer( v, pixmap, cmap );
   if (!b) {
         }
      }
         PUBLIC void
   glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap )
   {
      XMesaBuffer b = XMesaFindBuffer(dpy, pixmap);
   if (b) {
         }
   else if (getenv("MESA_DEBUG")) {
            }
         PUBLIC void
   glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
         {
      XMesaContext xm_src = src->xmesaContext;
   XMesaContext xm_dst = dst->xmesaContext;
   (void) dpy;
   if (GetCurrentContext() == src) {
         }
      }
         PUBLIC Bool
   glXQueryExtension( Display *dpy, int *errorBase, int *eventBase )
   {
      int op, ev, err;
   /* Mesa's GLX isn't really an X extension but we try to act like one. */
   if (!XQueryExtension(dpy, GLX_EXTENSION_NAME, &op, &ev, &err))
         if (errorBase)
         if (eventBase)
            }
         PUBLIC void
   glXDestroyContext( Display *dpy, GLXContext ctx )
   {
               if (glxCtx == NULL || glxCtx->xid == None)
            if (ctx->currentDpy) {
         } else {
      (void) dpy;
   XMesaDestroyContext( glxCtx->xmesaContext );
   XMesaGarbageCollect();
         }
         PUBLIC Bool
   glXIsDirect( Display *dpy, GLXContext ctx )
   {
         }
            PUBLIC void
   glXSwapBuffers( Display *dpy, GLXDrawable drawable )
   {
      XMesaBuffer buffer = XMesaFindBuffer( dpy, drawable );
            if (firsttime) {
      no_rast = getenv("SP_NO_RAST") != NULL;
               if (no_rast)
            if (buffer) {
         }
   else if (getenv("MESA_DEBUG")) {
      _mesa_warning(NULL, "glXSwapBuffers: invalid drawable 0x%x\n",
         }
            /*** GLX_MESA_copy_sub_buffer ***/
      PUBLIC void
   glXCopySubBufferMESA(Display *dpy, GLXDrawable drawable,
         {
      XMesaBuffer buffer = XMesaFindBuffer( dpy, drawable );
   if (buffer) {
         }
   else if (getenv("MESA_DEBUG")) {
            }
         PUBLIC Bool
   glXQueryVersion( Display *dpy, int *maj, int *min )
   {
      (void) dpy;
   /* Return GLX version, not Mesa version */
   assert(CLIENT_MAJOR_VERSION == SERVER_MAJOR_VERSION);
   *maj = CLIENT_MAJOR_VERSION;
   *min = MIN2( CLIENT_MINOR_VERSION, SERVER_MINOR_VERSION );
      }
         /*
   * Query the GLX attributes of the given XVisualInfo.
   */
   static int
   get_config( XMesaVisual xmvis, int attrib, int *value, GLboolean fbconfig )
   {
      assert(xmvis);
   switch(attrib) {
      case GLX_USE_GL:
      if (fbconfig)
      return 0;
         *value = xmvis->visinfo->depth;
   return 0;
         *value = 0;
   return 0;
         case GLX_RGBA:
      if (fbconfig)
      return 0;
         *value = (int) xmvis->mesa_visual.doubleBufferMode;
   return 0;
         *value = (int) xmvis->mesa_visual.stereoMode;
   return 0;
         *value = 0;
   return 0;
         case GLX_RED_SIZE:
   return 0;
         case GLX_GREEN_SIZE:
   return 0;
         case GLX_BLUE_SIZE:
   return 0;
         case GLX_ALPHA_SIZE:
   return 0;
         case GLX_DEPTH_SIZE:
   return 0;
         *value = xmvis->mesa_visual.stencilBits;
   return 0;
         *value = xmvis->mesa_visual.accumRedBits;
   return 0;
         *value = xmvis->mesa_visual.accumGreenBits;
   return 0;
         *value = xmvis->mesa_visual.accumBlueBits;
   return 0;
         case GLX_ACCUM_ALPHA_SIZE:
   return 0;
            /*
   * GLX_EXT_visual_info extension
   */
   case GLX_X_VISUAL_TYPE_EXT:
      switch (xmvis->visinfo->CLASS) {
      case StaticGray:   *value = GLX_STATIC_GRAY_EXT;   return 0;
   case GrayScale:    *value = GLX_GRAY_SCALE_EXT;    return 0;
   case StaticColor:  *value = GLX_STATIC_GRAY_EXT;   return 0;
   case PseudoColor:  *value = GLX_PSEUDO_COLOR_EXT;  return 0;
   case TrueColor:    *value = GLX_TRUE_COLOR_EXT;    return 0;
      }
      case GLX_TRANSPARENT_TYPE_EXT:
      /* normal planes */
   *value = GLX_NONE_EXT;
      case GLX_TRANSPARENT_INDEX_VALUE_EXT:
      /* undefined */
      case GLX_TRANSPARENT_RED_VALUE_EXT:
      /* undefined */
      case GLX_TRANSPARENT_GREEN_VALUE_EXT:
      /* undefined */
      case GLX_TRANSPARENT_BLUE_VALUE_EXT:
      /* undefined */
      case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
                  /*
   * GLX_EXT_visual_info extension
   */
   case GLX_VISUAL_CAVEAT_EXT:
                  /*
   * GLX_ARB_multisample
   */
   case GLX_SAMPLE_BUFFERS_ARB:
      *value = xmvis->mesa_visual.samples > 0;
      case GLX_SAMPLES_ARB:
                  /*
   * For FBConfigs:
   */
   case GLX_SCREEN_EXT:
      if (!fbconfig)
         *value = xmvis->visinfo->screen;
      case GLX_DRAWABLE_TYPE: /*SGIX too */
      if (!fbconfig)
         *value = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
      case GLX_RENDER_TYPE_SGIX:
      if (!fbconfig)
         *value = GLX_RGBA_BIT;
      case GLX_X_RENDERABLE_SGIX:
      if (!fbconfig)
         *value = True; /* XXX really? */
      case GLX_FBCONFIG_ID_SGIX:
      if (!fbconfig)
         *value = xmvis->visinfo->visualid;
      case GLX_MAX_PBUFFER_WIDTH:
      if (!fbconfig)
         /* XXX should be same as ctx->Const.MaxRenderbufferSize */
   *value = DisplayWidth(xmvis->display, xmvis->visinfo->screen);
      case GLX_MAX_PBUFFER_HEIGHT:
      if (!fbconfig)
         *value = DisplayHeight(xmvis->display, xmvis->visinfo->screen);
      case GLX_MAX_PBUFFER_PIXELS:
      if (!fbconfig)
         *value = DisplayWidth(xmvis->display, xmvis->visinfo->screen) *
            case GLX_VISUAL_ID:
      if (!fbconfig)
                     case GLX_BIND_TO_TEXTURE_RGB_EXT:
      *value = True; /*XXX*/
      case GLX_BIND_TO_TEXTURE_RGBA_EXT:
      /* XXX review */
   *value = xmvis->mesa_visual.alphaBits > 0 ? True : False;
      case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
      *value = True; /*XXX*/
      case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
      *value = (GLX_TEXTURE_1D_BIT_EXT |
                  case GLX_Y_INVERTED_EXT:
                  return GLX_BAD_ATTRIBUTE;
      }
      }
         PUBLIC int
   glXGetConfig( Display *dpy, XVisualInfo *visinfo,
         {
      XMesaVisual xmvis;
   int k;
   if (!dpy || !visinfo)
            xmvis = find_glx_visual( dpy, visinfo );
   if (!xmvis) {
      /* this visual wasn't obtained with glXChooseVisual */
   xmvis = create_glx_visual( dpy, visinfo );
   /* this visual can't be used for GL rendering */
   if (attrib==GLX_USE_GL) {
      *value = (int) False;
      }
   else {
         }
                     k = get_config(xmvis, attrib, value, GL_FALSE);
      }
         PUBLIC void
   glXWaitGL( void )
   {
      XMesaContext xmesa = XMesaGetCurrentContext();
      }
            PUBLIC void
   glXWaitX( void )
   {
      XMesaContext xmesa = XMesaGetCurrentContext();
      }
         static const char *
   get_extensions( void )
   {
         }
            /* GLX 1.1 and later */
   PUBLIC const char *
   glXQueryExtensionsString( Display *dpy, int screen )
   {
      (void) dpy;
   (void) screen;
      }
            /* GLX 1.1 and later */
   PUBLIC const char *
   glXQueryServerString( Display *dpy, int screen, int name )
   {
      static char version[1000];
   sprintf(version, "%d.%d %s",
            (void) dpy;
            switch (name) {
      case GLX_EXTENSIONS:
         return VENDOR;
         return version;
         default:
         }
            /* GLX 1.1 and later */
   PUBLIC const char *
   glXGetClientString( Display *dpy, int name )
   {
      static char version[1000];
   sprintf(version, "%d.%d %s", CLIENT_MAJOR_VERSION,
                     switch (name) {
      case GLX_EXTENSIONS:
         return VENDOR;
         return version;
         default:
         }
            /*
   * GLX 1.3 and later
   */
         PUBLIC int
   glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config,
         {
      XMesaVisual v = (XMesaVisual) config;
   (void) dpy;
            if (!dpy || !config || !value)
               }
         PUBLIC GLXFBConfig *
   glXGetFBConfigs( Display *dpy, int screen, int *nelements )
   {
      XVisualInfo *visuals, visTemplate;
   const long visMask = VisualScreenMask;
            /* Get list of all X visuals */
   visTemplate.screen = screen;
   visuals = XGetVisualInfo(dpy, visMask, &visTemplate, nelements);
   if (*nelements > 0) {
      XMesaVisual *results = malloc(*nelements * sizeof(XMesaVisual));
   if (!results) {
      *nelements = 0;
      }
   for (i = 0; i < *nelements; i++) {
      results[i] = create_glx_visual(dpy, visuals + i);
   if (!results[i]) {
      *nelements = i;
         }
      }
      }
         PUBLIC GLXFBConfig *
   glXChooseFBConfig(Display *dpy, int screen,
         {
               /* register ourselves as an extension on this display */
            if (!attribList || !attribList[0]) {
      /* return list of all configs (per GLX_SGIX_fbconfig spec) */
               xmvis = choose_visual(dpy, screen, attribList, GL_TRUE);
   if (xmvis) {
      GLXFBConfig *config = malloc(sizeof(XMesaVisual));
   if (!config) {
      *nitems = 0;
      }
   *nitems = 1;
   config[0] = (GLXFBConfig) xmvis;
      }
   else {
      *nitems = 0;
         }
         PUBLIC XVisualInfo *
   glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config )
   {
      if (dpy && config) {
      #if 0
         #else
         /* create a new vishandle - the cached one may be stale */
   xmvis->vishandle = malloc(sizeof(XVisualInfo));
   if (xmvis->vishandle) {
         }
   #endif
      }
   else {
            }
         PUBLIC GLXWindow
   glXCreateWindow(Display *dpy, GLXFBConfig config, Window win,
         {
      XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf;
   if (!xmvis)
            xmbuf = XMesaCreateWindowBuffer(xmvis, win);
   if (!xmbuf)
            (void) dpy;
               }
         PUBLIC void
   glXDestroyWindow( Display *dpy, GLXWindow window )
   {
      XMesaBuffer b = XMesaFindBuffer(dpy, (Drawable) window);
   if (b)
            }
         /* XXX untested */
   PUBLIC GLXPixmap
   glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pixmap,
         {
      XMesaVisual v = (XMesaVisual) config;
   XMesaBuffer b;
   const int *attr;
   int target = 0, format = 0, mipmap = 0;
            if (!dpy || !config || !pixmap)
            for (attr = attribList; attr && *attr; attr++) {
      switch (*attr) {
   case GLX_TEXTURE_FORMAT_EXT:
      attr++;
   switch (*attr) {
   case GLX_TEXTURE_FORMAT_NONE_EXT:
   case GLX_TEXTURE_FORMAT_RGB_EXT:
   case GLX_TEXTURE_FORMAT_RGBA_EXT:
      format = *attr;
      default:
      /* error */
      }
      case GLX_TEXTURE_TARGET_EXT:
      attr++;
   switch (*attr) {
   case GLX_TEXTURE_1D_EXT:
   case GLX_TEXTURE_2D_EXT:
   case GLX_TEXTURE_RECTANGLE_EXT:
      target = *attr;
      default:
      /* error */
      }
      case GLX_MIPMAP_TEXTURE_EXT:
      attr++;
   if (*attr)
            default:
      /* error */
                  if (format == GLX_TEXTURE_FORMAT_RGB_EXT) {
      if (get_config(v, GLX_BIND_TO_TEXTURE_RGB_EXT,
            || !value) {
         }
   else if (format == GLX_TEXTURE_FORMAT_RGBA_EXT) {
      if (get_config(v, GLX_BIND_TO_TEXTURE_RGBA_EXT,
            || !value) {
         }
   if (mipmap) {
      if (get_config(v, GLX_BIND_TO_MIPMAP_TEXTURE_EXT,
            || !value) {
         }
   if (target == GLX_TEXTURE_1D_EXT) {
      if (get_config(v, GLX_BIND_TO_TEXTURE_TARGETS_EXT,
            || (value & GLX_TEXTURE_1D_BIT_EXT) == 0) {
         }
   else if (target == GLX_TEXTURE_2D_EXT) {
      if (get_config(v, GLX_BIND_TO_TEXTURE_TARGETS_EXT,
            || (value & GLX_TEXTURE_2D_BIT_EXT) == 0) {
         }
   if (target == GLX_TEXTURE_RECTANGLE_EXT) {
      if (get_config(v, GLX_BIND_TO_TEXTURE_TARGETS_EXT,
            || (value & GLX_TEXTURE_RECTANGLE_BIT_EXT) == 0) {
                  if (format || target || mipmap) {
      /* texture from pixmap */
      }
   else {
         }
   if (!b) {
                     }
         PUBLIC void
   glXDestroyPixmap( Display *dpy, GLXPixmap pixmap )
   {
      XMesaBuffer b = XMesaFindBuffer(dpy, (Drawable)pixmap);
   if (b)
            }
         PUBLIC GLXPbuffer
   glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attribList)
   {
      XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf;
   const int *attrib;
   int width = 0, height = 0;
                     for (attrib = attribList; *attrib; attrib++) {
      switch (*attrib) {
      case GLX_PBUFFER_WIDTH:
      attrib++;
   width = *attrib;
      case GLX_PBUFFER_HEIGHT:
      attrib++;
   height = *attrib;
      case GLX_PRESERVED_CONTENTS:
      attrib++;
   preserveContents = *attrib;
      case GLX_LARGEST_PBUFFER:
      attrib++;
   useLargest = *attrib;
      default:
                  if (width == 0 || height == 0)
            if (width > PBUFFER_MAX_SIZE || height > PBUFFER_MAX_SIZE) {
      /* If allocation would have failed and GLX_LARGEST_PBUFFER is set,
   * allocate the largest possible buffer.
   */
   if (useLargest) {
      width = PBUFFER_MAX_SIZE;
                  xmbuf = XMesaCreatePBuffer( xmvis, 0, width, height);
   /* A GLXPbuffer handle must be an X Drawable because that's what
   * glXMakeCurrent takes.
   */
   if (xmbuf) {
      xmbuf->largestPbuffer = useLargest;
   xmbuf->preservedContents = preserveContents;
      }
   else {
            }
         PUBLIC void
   glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf )
   {
      XMesaBuffer b = XMesaFindBuffer(dpy, pbuf);
   if (b) {
            }
         PUBLIC void
   glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute,
         {
      GLuint width, height;
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, draw);
   if (!xmbuf) {
      generate_error(dpy, GLXBadDrawable, draw, X_GLXGetDrawableAttributes, False);
               /* make sure buffer's dimensions are up to date */
            switch (attribute) {
      case GLX_WIDTH:
      *value = width;
      case GLX_HEIGHT:
      *value = height;
      case GLX_PRESERVED_CONTENTS:
      *value = xmbuf->preservedContents;
      case GLX_LARGEST_PBUFFER:
      *value = xmbuf->largestPbuffer;
      case GLX_FBCONFIG_ID:
      *value = xmbuf->xm_visual->visinfo->visualid;
      case GLX_TEXTURE_FORMAT_EXT:
      *value = xmbuf->TextureFormat;
      case GLX_TEXTURE_TARGET_EXT:
      *value = xmbuf->TextureTarget;
      case GLX_MIPMAP_TEXTURE_EXT:
                  default:
      generate_error(dpy, BadValue, 0, X_GLXCreateContextAttribsARB, true);
      }
         PUBLIC GLXContext
   glXCreateNewContext( Display *dpy, GLXFBConfig config,
         {
               if (!dpy || !config ||
      (renderType != GLX_RGBA_TYPE && renderType != GLX_COLOR_INDEX_TYPE))
         return create_context(dpy, xmvis,
                  }
         PUBLIC int
   glXQueryContext( Display *dpy, GLXContext ctx, int attribute, int *value )
   {
      GLXContext glxCtx = ctx;
            (void) dpy;
            switch (attribute) {
   case GLX_FBCONFIG_ID:
      *value = xmctx->xm_visual->visinfo->visualid;
      case GLX_RENDER_TYPE:
      *value = GLX_RGBA_TYPE;
      case GLX_SCREEN:
      *value = 0;
      default:
         }
      }
         PUBLIC void
   glXSelectEvent( Display *dpy, GLXDrawable drawable, unsigned long mask )
   {
      XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf)
      }
         PUBLIC void
   glXGetSelectedEvent(Display *dpy, GLXDrawable drawable, unsigned long *mask)
   {
      XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf)
         else
      }
            /*** GLX_SGI_swap_control ***/
      PUBLIC int
   glXSwapIntervalSGI(int interval)
   {
      (void) interval;
      }
            /*** GLX_SGI_video_sync ***/
      static unsigned int FrameCounter = 0;
      PUBLIC int
   glXGetVideoSyncSGI(unsigned int *count)
   {
      /* this is a bogus implementation */
   *count = FrameCounter++;
      }
      PUBLIC int
   glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
   {
      if (divisor <= 0 || remainder < 0)
         /* this is a bogus implementation */
   FrameCounter++;
   while (FrameCounter % divisor != remainder)
         *count = FrameCounter;
      }
            /*** GLX_SGI_make_current_read ***/
      PUBLIC Bool
   glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read,
         {
         }
      /* not used
   static GLXDrawable
   glXGetCurrentReadDrawableSGI(void)
   {
         }
   */
         /*** GLX_SGIX_video_source ***/
   #if defined(_VL_H)
      PUBLIC GLXVideoSourceSGIX
   glXCreateGLXVideoSourceSGIX(Display *dpy, int screen, VLServer server,
         {
      (void) dpy;
   (void) screen;
   (void) server;
   (void) path;
   (void) nodeClass;
   (void) drainNode;
      }
      PUBLIC void
   glXDestroyGLXVideoSourceSGIX(Display *dpy, GLXVideoSourceSGIX src)
   {
      (void) dpy;
      }
      #endif
         /*** GLX_EXT_import_context ***/
      PUBLIC void
   glXFreeContextEXT(Display *dpy, GLXContext context)
   {
      (void) dpy;
      }
      PUBLIC GLXContextID
   glXGetContextIDEXT(const GLXContext context)
   {
      (void) context;
      }
      PUBLIC GLXContext
   glXImportContextEXT(Display *dpy, GLXContextID contextID)
   {
      (void) dpy;
   (void) contextID;
      }
      PUBLIC int
   glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute,
         {
      (void) dpy;
   (void) context;
   (void) attribute;
   (void) value;
      }
            /*** GLX_SGIX_fbconfig ***/
      PUBLIC int
   glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config,
         {
         }
      PUBLIC GLXFBConfigSGIX *
   glXChooseFBConfigSGIX(Display *dpy, int screen, int *attrib_list,
         {
      return (GLXFBConfig *) glXChooseFBConfig(dpy, screen,
      }
         PUBLIC GLXPixmap
   glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config,
         {
      XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf = XMesaCreatePixmapBuffer(xmvis, pixmap, 0);
      }
         PUBLIC GLXContext
   glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config,
               {
               if (!dpy || !config ||
      (renderType != GLX_RGBA_TYPE && renderType != GLX_COLOR_INDEX_TYPE))
         return create_context(dpy, xmvis,
                  }
         PUBLIC XVisualInfo *
   glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
   {
         }
         PUBLIC GLXFBConfigSGIX
   glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
   {
      XMesaVisual xmvis = find_glx_visual(dpy, vis);
   if (!xmvis) {
      /* This visual wasn't found with glXChooseVisual() */
                  }
            /*** GLX_SGIX_pbuffer ***/
      PUBLIC GLXPbufferSGIX
   glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config,
               {
      XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf;
   const int *attrib;
                     for (attrib = attribList; attrib && *attrib; attrib++) {
      switch (*attrib) {
      case GLX_PRESERVED_CONTENTS_SGIX:
      attrib++;
   preserveContents = *attrib; /* ignored */
      case GLX_LARGEST_PBUFFER_SGIX:
      attrib++;
   useLargest = *attrib; /* ignored */
      default:
                  /* not used at this time */
   (void) useLargest;
            xmbuf = XMesaCreatePBuffer( xmvis, 0, width, height);
   /* A GLXPbuffer handle must be an X Drawable because that's what
   * glXMakeCurrent takes.
   */
      }
         PUBLIC void
   glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf)
   {
      XMesaBuffer xmbuf = XMesaFindBuffer(dpy, pbuf);
   if (xmbuf) {
            }
         PUBLIC void
   glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf, int attribute,
         {
               if (!xmbuf) {
      /* Generate GLXBadPbufferSGIX for bad pbuffer */
               switch (attribute) {
      case GLX_PRESERVED_CONTENTS_SGIX:
      *value = True;
      case GLX_LARGEST_PBUFFER_SGIX:
      *value = xmesa_buffer_width(xmbuf) * xmesa_buffer_height(xmbuf);
      case GLX_WIDTH_SGIX:
      *value = xmesa_buffer_width(xmbuf);
      case GLX_HEIGHT_SGIX:
      *value = xmesa_buffer_height(xmbuf);
      case GLX_EVENT_MASK_SGIX:
      *value = 0;  /* XXX might be wrong */
      default:
         }
         PUBLIC void
   glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
   {
      XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf) {
      /* Note: we'll never generate clobber events */
         }
         PUBLIC void
   glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable,
         {
      XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf) {
         }
   else {
            }
            /*** GLX_SGI_cushion ***/
      PUBLIC void
   glXCushionSGI(Display *dpy, Window win, float cushion)
   {
      (void) dpy;
   (void) win;
      }
            /*** GLX_SGIX_video_resize ***/
      PUBLIC int
   glXBindChannelToWindowSGIX(Display *dpy, int screen, int channel,
         {
      (void) dpy;
   (void) screen;
   (void) channel;
   (void) window;
      }
      PUBLIC int
   glXChannelRectSGIX(Display *dpy, int screen, int channel,
         {
      (void) dpy;
   (void) screen;
   (void) channel;
   (void) x;
   (void) y;
   (void) w;
   (void) h;
      }
      PUBLIC int
   glXQueryChannelRectSGIX(Display *dpy, int screen, int channel,
         {
      (void) dpy;
   (void) screen;
   (void) channel;
   (void) x;
   (void) y;
   (void) w;
   (void) h;
      }
      PUBLIC int
   glXQueryChannelDeltasSGIX(Display *dpy, int screen, int channel,
         {
      (void) dpy;
   (void) screen;
   (void) channel;
   (void) dx;
   (void) dy;
   (void) dw;
   (void) dh;
      }
      PUBLIC int
   glXChannelRectSyncSGIX(Display *dpy, int screen, int channel, GLenum synctype)
   {
      (void) dpy;
   (void) screen;
   (void) channel;
   (void) synctype;
      }
            /*** GLX_SGIX_dmbuffer **/
      #if defined(_DM_BUFFER_H_)
   PUBLIC Bool
   glXAssociateDMPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuffer,
         {
      (void) dpy;
   (void) pbuffer;
   (void) params;
   (void) dmbuffer;
      }
   #endif
         /*** GLX_SUN_get_transparent_index ***/
      PUBLIC Status
   glXGetTransparentIndexSUN(Display *dpy, Window overlay, Window underlay,
         {
      (void) dpy;
   (void) overlay;
   (void) underlay;
   (void) pTransparent;
      }
            /*** GLX_MESA_release_buffers ***/
      /*
   * Release the depth, stencil, accum buffers attached to a GLXDrawable
   * (a window or pixmap) prior to destroying the GLXDrawable.
   */
   PUBLIC Bool
   glXReleaseBuffersMESA( Display *dpy, GLXDrawable d )
   {
      XMesaBuffer b = XMesaFindBuffer(dpy, d);
   if (b) {
      XMesaDestroyBuffer(b);
      }
      }
      /*** GLX_EXT_texture_from_pixmap ***/
      PUBLIC void
   glXBindTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer,
         {
      XMesaBuffer b = XMesaFindBuffer(dpy, drawable);
   if (b)
      }
      PUBLIC void
   glXReleaseTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer)
   {
      XMesaBuffer b = XMesaFindBuffer(dpy, drawable);
   if (b)
      }
            /*** GLX_ARB_create_context ***/
         GLXContext
   glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config,
               {
      XMesaVisual xmvis = (XMesaVisual) config;
   int majorVersion = 1, minorVersion = 0;
   int contextFlags = 0x0;
   int profileMask = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
   int renderType = GLX_RGBA_TYPE;
   unsigned i;
   Bool done = False;
   const int contextFlagsAll = (GLX_CONTEXT_DEBUG_BIT_ARB |
                  /* parse attrib_list */
   for (i = 0; !done && attrib_list && attrib_list[i]; i++) {
      switch (attrib_list[i]) {
   case GLX_CONTEXT_MAJOR_VERSION_ARB:
      majorVersion = attrib_list[++i];
      case GLX_CONTEXT_MINOR_VERSION_ARB:
      minorVersion = attrib_list[++i];
      case GLX_CONTEXT_FLAGS_ARB:
      contextFlags = attrib_list[++i];
      case GLX_CONTEXT_PROFILE_MASK_ARB:
      profileMask = attrib_list[++i];
      case GLX_RENDER_TYPE:
      renderType = attrib_list[++i];
      case 0:
      /* end of list */
   done = True;
      default:
      /* bad attribute */
   generate_error(dpy, BadValue, 0, X_GLXCreateContextAttribsARB, True);
                  /* check contextFlags */
   if (contextFlags & ~contextFlagsAll) {
      generate_error(dpy, BadValue, 0, X_GLXCreateContextAttribsARB, True);
               /* check profileMask */
   if (profileMask != GLX_CONTEXT_CORE_PROFILE_BIT_ARB &&
      profileMask != GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB &&
   profileMask != GLX_CONTEXT_ES_PROFILE_BIT_EXT) {
   generate_error(dpy, GLXBadProfileARB, 0, X_GLXCreateContextAttribsARB, False);
               /* check renderType */
   if (renderType != GLX_RGBA_TYPE &&
      renderType != GLX_COLOR_INDEX_TYPE) {
   generate_error(dpy, BadValue, 0, X_GLXCreateContextAttribsARB, True);
               /* check version */
   if (majorVersion <= 0 ||
      minorVersion < 0 ||
   (profileMask != GLX_CONTEXT_ES_PROFILE_BIT_EXT &&
   ((majorVersion == 1 && minorVersion > 5) ||
      (majorVersion == 2 && minorVersion > 1) ||
   (majorVersion == 3 && minorVersion > 3) ||
   (majorVersion == 4 && minorVersion > 5) ||
      generate_error(dpy, BadMatch, 0, X_GLXCreateContextAttribsARB, True);
      }
   if (profileMask == GLX_CONTEXT_ES_PROFILE_BIT_EXT &&
      ((majorVersion == 1 && minorVersion > 1) ||
   (majorVersion == 2 && minorVersion > 0) ||
   (majorVersion == 3 && minorVersion > 1) ||
   majorVersion > 3)) {
   /* GLX_EXT_create_context_es2_profile says nothing to justifying a
   * different error code for invalid ES versions, but this is what NVIDIA
   * does and piglit expects.
   */
   generate_error(dpy, GLXBadProfileARB, 0, X_GLXCreateContextAttribsARB, False);
               if ((contextFlags & GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB) &&
      majorVersion < 3) {
   generate_error(dpy, BadMatch, 0, X_GLXCreateContextAttribsARB, True);
               if (renderType == GLX_COLOR_INDEX_TYPE && majorVersion >= 3) {
      generate_error(dpy, BadMatch, 0, X_GLXCreateContextAttribsARB, True);
               ctx = create_context(dpy, xmvis,
                           if (!ctx) {
                     }
